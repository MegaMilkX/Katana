#ifndef AUDIO_CONTROLLER_HPP
#define AUDIO_CONTROLLER_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"

#include "../../components/audio_source.hpp"
#include "../../components/audio_listener.hpp"

#include "../../../common/util/imgui_helpers.hpp"

class AudioController : public SceneControllerEventFilter<AudioSource, AudioListener> {
    RTTR_ENABLE(SceneController)
public:
    virtual void onAttribCreated(AudioSource* s) { sources.insert(s); }
    virtual void onAttribDeleted(AudioSource* s) { sources.erase(s); }
    virtual void onAttribDeleted(AudioListener* l) { 
        if(l == listener) {
            listener = 0;
        } 
    }

    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_DYNAMICS + 1 };
    }

    virtual void copy(SceneController& othr) {
        AudioController& other = (AudioController&)othr;
        if(other.listener) {
            listener = scene->findComponent<AudioListener>(other.listener->getOwner()->getName());
        }
    }

    virtual void init(GameScene* scn) {
        scene = scn;
    }
    ~AudioController() {
    }

    virtual void onStart() {
        for(auto s : sources) {
            if(s->isAutoplay()) {
                s->play(true);
            }
        }
    }

    virtual void onUpdate() {
        // TODO: Synchronize
        if(listener) {
            audio().setListenerTransform(listener->getOwner()->getTransform()->getWorldTransform());
        } else {
            audio().setListenerTransform(gfxm::mat4(1.0f));
        }
        for(auto s : sources) {
            s->_updateTransform();
        }
    }

    virtual void onEnd() {
        for(auto s : sources) {
            s->stop();
        }
    }

    void setListener(AudioListener* l) {
        listener = l;
    }
    AudioListener* getListener() {
        return listener;
    }

    virtual void onGui() {
        imguiComponentCombo(
            "Listener",
            listener,
            scene
        );
    }

    virtual void serialize(out_stream& out) {
        DataWriter w(&out);
        if(listener) {
            w.write(listener->getOwner()->getName());
        } else {
            w.write(std::string());
        }
    }
    virtual void deserialize(in_stream& in) {
        DataReader r(&in);
        listener = scene->findComponent<AudioListener>(r.readStr());
    }
private:
    virtual void onComponentRemoved(Attribute* c) {
        if(c == listener) {
            listener = 0;
        }
    }

    std::set<AudioSource*> sources;
    AudioListener* listener = 0;
    GameScene* scene = 0;
};
STATIC_RUN(AudioController) {
    rttr::registration::class_<AudioController>("AudioController")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
