#ifndef AUDIO_SOURCE_HPP
#define AUDIO_SOURCE_HPP

#include "attribute.hpp"
#include "../../common/gfxm.hpp"
#include "../scene/node.hpp"

#include "../../common/resource/audio_clip.hpp"

#include "../../common/resource/resource_tree.hpp"

#include "../../common/audio.hpp"

#include "../../common/util/has_suffix.hpp"

#include "../../common/util/imgui_helpers.hpp"

class AudioSource : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    AudioSource() {
        emid = audio().createChannel();
    }
    ~AudioSource();

    virtual void onCreate();

    virtual void copy(Attribute* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        AudioSource* o = (AudioSource*)other;
        setClip(o->clip);
        autoplay = o->autoplay;
        setLooping(o->looping);
    }

    void play(bool reset_cursor = false) {
        audio().resetCursor(emid);
        audio().play3d(emid);
    }
    void stop() {
        audio().stop(emid);
    }

    void setClip(std::shared_ptr<AudioClip> c) {
        if(c) {
            audio().setBuffer(emid, c->getBuffer());
        }
        audio().stop(emid);
        clip = c;
    }
    std::shared_ptr<AudioClip> getClip() {
        return clip;
    }

    void setLooping(bool v) {
        looping = v;
        audio().setLooping(emid, looping);
    }

    bool isAutoplay() {
        return autoplay;
    }

    void _updateTransform() {
        audio().setPosition(emid, getOwner()->getTransform()->getWorldPosition());
    }

    virtual void onGui() {
        std::string clip_name = "<null>";
        if(clip) clip_name = clip->Name();
        
        imguiResourceTreeCombo("AudioClip", clip, "ogg", [this](){
            setClip(clip);
        });
        
        // TODO: FIX
        /*
        auto clip_list = std::vector<std::string>();// GlobalDataRegistry().makeList(".ogg");
        std::string clip_name = "<null>";
        if(clip) {
            clip_name = clip->Name();
        }
        if(ImGui::BeginCombo(MKSTR("AudioClip").c_str(), clip_name.c_str())) {
            for(auto& n : clip_list) {
                if(ImGui::Selectable(n.c_str(), n == clip_name)) {
                    setClip(retrieve<AudioClip>(n));
                }
            }
            ImGui::EndCombo();
        }
         */
        if(ImGui::Button("Preview")) {
            audio().play(emid);
        }
        if(audio().isPlaying(emid)) {
            ImGui::SameLine();
            if(ImGui::Button("Stop")) {
                audio().stop(emid);
            }
        }
        bool is_looping = audio().isLooping(emid);
        if(ImGui::Checkbox("Loop", &is_looping)) {
            setLooping(is_looping);
        }
        if(ImGui::Checkbox("Autoplay", &autoplay)) {
        }
    }

    virtual bool serialize(out_stream& out) {
        DataWriter w(&out);
        if(clip) {
            w.write(clip->Name());
        } else {
            w.write(std::string());
        }
        w.write<uint8_t>(autoplay);
        w.write(volume);
        w.write<uint8_t>(looping);
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        DataReader r(&in);
        std::string clip_name = r.readStr();
        if(!clip_name.empty()) {
            setClip(retrieve<AudioClip>(clip_name));
        }
        autoplay = (bool)r.read<uint8_t>();
        volume = r.read<float>();
        setLooping((bool)r.read<uint8_t>());
        return true;
    }

    virtual const char* getIconCode() const { return ICON_MDI_VOLUME_HIGH; }
private:
    size_t emid;
    std::shared_ptr<AudioClip> clip;
    bool autoplay = false;
    float volume = 1.0f;
    bool looping = false;
};

#endif
