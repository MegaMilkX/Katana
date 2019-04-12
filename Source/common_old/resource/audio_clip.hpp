#ifndef AUDIO_CLIP_HPP
#define AUDIO_CLIP_HPP

#include "resource.h"
#include "../util/audio/audio_mixer.hpp"

#define STB_VORBIS_HEADER_ONLY
extern "C" {
    #include "../lib/stb_vorbis.c"
}

class AudioClip : public Resource {
    RTTR_ENABLE(Resource)
public:
    AudioBuffer* getBuffer() {
        return buf.get();
    }

    virtual void serialize(out_stream& out) {

    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        auto vec = in.read<unsigned char>(sz);
        
        int channels = 2;
        int sampleRate = 44100;
        short* decoded;
        int len;
        len = stb_vorbis_decode_memory(vec.data(), vec.size(), &channels, &sampleRate, &decoded);
        
        buf.reset(new AudioBuffer(
            decoded, len * sizeof(short) * channels, sampleRate, channels
        ));

        free(decoded);
        return true;
    }
private:
    std::shared_ptr<AudioBuffer> buf;
};
STATIC_RUN(AudioClip) {
    rttr::registration::class_<AudioClip>("AudioClip")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
