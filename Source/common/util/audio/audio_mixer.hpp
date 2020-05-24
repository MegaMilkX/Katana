#ifndef AUDIO_MIXER_HPP
#define AUDIO_MIXER_HPP

#include <thread>
#include <mutex>


#include "../object_pool.hpp"

#include <stdint.h>
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")

#include "../../gfxm.hpp"
#include "../log.hpp"

#include "../../lib/imgui_wrap.hpp"

#define STB_VORBIS_HEADER_ONLY
extern "C" {
    #include "../../lib/stb_vorbis.c"
}

#define AUDIO_BUFFER_SZ 256

#include "audio_buffer.hpp"

static const int SHORT_MAX = std::numeric_limits<short>().max();

struct AudioChannel {
    size_t cursor = 0;
    AudioBuffer* buf = 0;
    float volume = 1.0f;
    float panning = 0.0f;
    gfxm::vec3 pos;
    bool looping = false;

    void setPosition(const gfxm::vec3& p) {
        pos = p;
    }
};

class AudioMixer : public IXAudio2VoiceCallback {
public:
    AudioMixer() {}
    void setListenerTransform(const gfxm::mat4& t) {
        std::lock_guard<std::mutex> lock(sync);
        lis_transform = t;
    }

    size_t createChannel() {
        return emitter_pool.acquire();
    }
    void freeChannel(size_t chid) {
        stop(chid);
        emitter_pool.free(chid);
    }

    void play(size_t ch) {
        emitters.insert(ch);
    }
    void play3d(size_t ch) {
        emitters3d.insert(ch);
    }
    void stop(size_t ch) {
        emitters.erase(ch);
        emitters3d.erase(ch);
    }
    void resetCursor(size_t ch) {
        emitter_pool.deref(ch)->cursor = 0;
    }

    void setBuffer(size_t ch, AudioBuffer* buf) {
        emitter_pool.deref(ch)->buf = buf;
        emitter_pool.deref(ch)->cursor = 0;
    }
    void setLooping(size_t ch, bool v) {
        emitter_pool.deref(ch)->looping = v;
    }
    void setPosition(size_t ch, const gfxm::vec3& pos) {
        std::lock_guard<std::mutex> lock(sync);
        emitter_pool.deref(ch)->setPosition(pos);
    }

    bool isPlaying(size_t ch) {
        return emitters.count(ch) || emitters3d.count(ch);
    }
    bool isLooping(size_t ch) {
        return emitter_pool.deref(ch)->looping;
    }

    void playOnce(AudioBuffer* buf, float vol = 1.0f, float pan = .0f) {
        size_t em = emitter_pool.acquire();
        emitter_pool.deref(em)->buf = buf;
        emitter_pool.deref(em)->volume = vol;
        emitter_pool.deref(em)->panning = pan;
        emitters.insert(em);
    }
    void playOnce3d(AudioBuffer* buf, const gfxm::vec3& pos, float vol = 1.0f) {
        size_t em = emitter_pool.acquire();
        auto emp = emitter_pool.deref(em);
        emp->buf = buf;
        emp->volume = vol;
        emp->setPosition(pos);
        emitters3d.insert(em);
    }

    bool init(int sampleRate, int bps) {
        this->sampleRate = sampleRate;
        this->bitPerSample = bps;
        this->nChannels = 2;
        //memset(buffer, 0, sizeof(buffer));
        int blockAlign = (bitPerSample * nChannels) / 8;

        front = &buffer[0];
        back = &buffer_back[0];

        gfxm::mat4 lis_transform = gfxm::mat4(1.0f);

        WAVEFORMATEX wfx = {
            WAVE_FORMAT_PCM,
            (WORD)nChannels,
            (DWORD)sampleRate,
            (DWORD)(sampleRate * blockAlign),
            (WORD)blockAlign,
            (WORD)bitPerSample,
            0
        };

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if(FAILED(hr)) {
            LOG_ERR("Failed to init COM: " << hr);
            return false;
        }
        #if(_WIN32_WINNT < 0x602)
        #ifdef _DEBUG
            HMODULE xAudioDll = LoadLibraryExW(L"XAudioD2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        #else
            HMODULE xAudioDll = LoadLibraryExW(L"XAudio2_7.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        #endif
            if(!xAudioDll) {
                LOG_ERR("Failed to find XAudio2.7 dll");
                CoUninitialize();
                return 1;
            }
        #endif
        UINT32 flags = 0;
        #if (_WIN32_WINNT < 0x0602 /*_WIN32_WINNT_WIN8*/) && defined(_DEBUG)
            flags |= XAUDIO2_DEBUG_ENGINE;
        #endif

        hr = XAudio2Create(&pXAudio2, flags);
        if(FAILED(hr)) {
            LOG_ERR("Failed to init XAudio2: " << hr);
            CoUninitialize();
            return false;
        }

        if(FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
        {
            LOG_ERR("Failed to create mastering voice: " << hr);
            //pXAudio2.Reset();
            CoUninitialize();
            return false;
        }
        if(FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &wfx, 0, 1.0f, this)))
        {
            LOG_ERR("Failed to create source voice: " << hr);
            return false;
        }

        memset(buffer, 0, sizeof(buffer));
    
        pMasteringVoice->SetVolume(1.00f);
/*
        for(size_t i = 0; i < 88200 - (nChannels - 1); i += nChannels) {
            float normal_pos = i / (float)sizeof(buffer);
            float a = -gfxm::pi + normal_pos * gfxm::pi * 2;
            buffer[i] = sinf(a * 150) * 32767;
            buffer[i + 1] = sinf(a * 150) * 32767;
        }
*/        
        
        XAUDIO2_BUFFER buf = { 0 };
        buf.AudioBytes = sizeof(buffer);
        buf.pAudioData = (BYTE*)front;
        buf.LoopCount = 0;

        hr = pSourceVoice->SubmitSourceBuffer(&buf);
        pSourceVoice->Start(0, 0);

        CoUninitialize();

        return true;
    }
    void cleanup() {
        pXAudio2->StopEngine();
        pXAudio2->Release();
    }

    void debugGui() {        
        for(size_t i = 0; i < AUDIO_BUFFER_SZ; ++i) {
            buffer_float[i] = buffer[i] / 32767.0f;
        }
        
        for(size_t i = 0; i < AUDIO_BUFFER_SZ; ++i) {
            buffer_back_float[i] = buffer_back[i] / 32767.0f;
        }

        if(ImGui::Begin("Audio_test")) {
            ImGui::PlotHistogram("buffer", buffer_float, AUDIO_BUFFER_SZ, 0, NULL, -1.0f, 1.0f, ImVec2(0,80));
            ImGui::PlotHistogram("buffer_back", buffer_back_float, AUDIO_BUFFER_SZ, 0, NULL, -1.0f, 1.0f, ImVec2(0,80));
        }
        ImGui::End();
    }

    void __stdcall OnStreamEnd() {   }

    //Unused methods are stubs
    void __stdcall OnVoiceProcessingPassEnd() { }
    void __stdcall OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
    void __stdcall OnBufferEnd(void * pBufferContext) {
        gfxm::mat4 lis_trans_copy;
        {
            std::lock_guard<std::mutex> lock(sync);
            lis_trans_copy = lis_transform;
        }

        memset(buffer_f, 0, sizeof(buffer_f));
        memset(back, 0, sizeof(buffer));
        size_t buf_len = sizeof(buffer_f) / sizeof(buffer_f[0]);
        for(auto& it = emitters.begin(); it != emitters.end();) {
            size_t ei = (*it);
            ++it;
            AudioChannel* em = emitter_pool.deref(ei);
            if(!em->buf) {
                emitters.erase(ei);
                continue;
            }

            size_t src_cur = em->cursor;
            short* data = em->buf->getPtr();
            size_t src_len = em->buf->sampleCount();
            
            size_t advance = mix(
                buffer_f, buf_len, data, src_len, 
                src_cur, em->volume, em->panning
            );
            
            em->cursor += advance;
            if(em->cursor >= em->buf->sampleCount()) {
                if(!em->looping) {
                    emitters.erase(ei);
                }
            } 
            em->cursor = em->cursor % src_len;
        }
        for(auto& it = emitters3d.begin(); it != emitters3d.end();) {
            size_t ei = (*it);
            ++it;
            AudioChannel* em = emitter_pool.deref(ei);
            if(!em->buf) {
                emitters3d.erase(ei);
                continue;
            }

            gfxm::vec3 p_;
            {
                std::lock_guard<std::mutex> lock(sync);
                p_ = em->pos;
            }
            size_t advance = mix3d(
                buffer_f, buf_len,
                em->buf->getPtr(),
                em->buf->sampleCount(),
                em->cursor, em->buf->channelCount(), em->volume,
                p_,
                lis_trans_copy
            );

            em->cursor += advance;
            if(em->cursor >= em->buf->sampleCount()) {
                if(!em->looping) {
                    emitters3d.erase(ei);
                }
            }
            em->cursor = em->cursor % em->buf->sampleCount();
        }

        for(int i = 0; i < AUDIO_BUFFER_SZ; ++i) {
            //samplef = pow(samplef, pow_) * sign;
            back[i] = SHORT_MAX * gfxm::_min(1.0f, (buffer_f[i] * 0.5f));
        }

        short* tmp = front;
        front = back;
        back = tmp;
        
        XAUDIO2_BUFFER buf = { 0 };
        buf.AudioBytes = sizeof(buffer);
        buf.pAudioData = (BYTE*)front;
        buf.LoopCount = 0;
        pSourceVoice->SubmitSourceBuffer(&buf);
    }
    void __stdcall OnBufferStart(void * pBufferContext) {    }
    void __stdcall OnLoopEnd(void * pBufferContext) {
        
    }
    void __stdcall OnVoiceError(void * pBufferContext, HRESULT Error) {
        LOG("Voice error: " << Error);
     }
private:
    size_t mix(
        float* dest, 
        size_t dest_len, 
        short* src, 
        size_t src_len,
        size_t cur,
        float vol,
        float panning
    ) {
        size_t sample_len = src_len < dest_len ? src_len : dest_len;
        size_t overflow = (cur + sample_len) > src_len ? (cur + sample_len) - src_len : 0;

        size_t tgt0sz = sample_len - overflow;
        size_t tgt1sz = overflow;
        short* tgt0 = src + cur;
        short* tgt1 = src;

        float mul = 1.0f / (float)SHORT_MAX;

        for(size_t i = 0; i < tgt0sz; ++i) {
            int lr = (i % 2) * 2 - 1;
            float pan = std::min(fabs(lr + panning), 1.0f);
            
            dest[i] += tgt0[i] * mul * vol * pan;
        }
        for(size_t i = 0; i < tgt1sz; ++i) {
            int lr = (i % 2) * 2 - 1;
            float pan = std::min(fabs(lr + panning), 1.0f);
            (dest + tgt0sz)[i] += tgt1[i] * mul * vol * pan;
        }

        return sample_len;
    }
    size_t mix3d(
        float* dest, 
        size_t dest_len, 
        short* src, 
        size_t src_len,
        size_t cur,
        int channels,
        float vol,
        const gfxm::vec3& pos,
        const gfxm::mat4& lis_trans_copy
    ) {
        size_t sample_len = src_len < dest_len ? src_len : dest_len;
        size_t overflow = (cur + sample_len) > src_len ? (cur + sample_len) - src_len : 0;

        size_t tgt0sz = sample_len - overflow;
        size_t tgt1sz = overflow;
        short* tgt0 = src + cur;
        short* tgt1 = src;

        gfxm::vec3 ears[2] = {
            gfxm::vec3(-0.1075f, .0f, .0f),
            gfxm::vec3( 0.1075f, .0f, .0f)
        };
        ears[0] = lis_trans_copy * gfxm::vec4(ears[0], 1.0f);
        ears[1] = lis_trans_copy * gfxm::vec4(ears[1], 1.0f);
        float falloff[2] = {
            std::min(1.0f / pow(gfxm::length(ears[0] - pos), 2.0f), 1.0f),
            std::min(1.0f / pow(gfxm::length(ears[1] - pos), 2.0f), 1.0f)
        };

        float mul = 1.0f / (float)SHORT_MAX;

        for(size_t i = 0; i < tgt0sz; ++i) {
            int lr = (i % 2);            
            dest[i] += tgt0[i] * mul * vol * falloff[lr];
        }
        for(size_t i = 0; i < tgt1sz; ++i) {
            int lr = (i % 2);
            (dest + tgt0sz)[i] += tgt1[i] * mul * vol * falloff[lr];
        }

        return sample_len;
    }

    int sampleRate;
    int bitPerSample;
    int nChannels;

    short* front;
    short* back;
    
    float buffer_f[AUDIO_BUFFER_SZ];
    short buffer[AUDIO_BUFFER_SZ];
    short buffer_back[AUDIO_BUFFER_SZ];

    float buffer_float[AUDIO_BUFFER_SZ];
    float buffer_back_float[AUDIO_BUFFER_SZ];

    IXAudio2* pXAudio2;
    IXAudio2MasteringVoice* pMasteringVoice = 0;
    IXAudio2SourceVoice* pSourceVoice;

    std::set<size_t> emitters;
    std::set<size_t> emitters3d;
    ObjectPool<AudioChannel> emitter_pool;
    ObjectPool<AudioBuffer> buffer_pool;

    gfxm::mat4 lis_transform;
    std::mutex sync;
};

#endif
