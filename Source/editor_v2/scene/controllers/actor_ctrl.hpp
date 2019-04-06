#ifndef ACTOR_CTRL_HPP
#define ACTOR_CTRL_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"

#include "../actor_object.hpp"

class ActorCtrl : public SceneController {
    RTTR_ENABLE(SceneController)
public:
    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_ACTORS };
    }
    virtual void onStart() {
        for(auto a : actors) {
            a->reset();
        }
    }
    virtual void onUpdate() {
        for(auto a : actors) {
            a->update();
        }
    }

    void _regActor(ActorObject* a) {
        actors.insert(a);
        a->init();
    }
    void _unregActor(ActorObject* a) {
        
        actors.erase(a);
    }
private:
    std::set<ActorObject*> actors;
};
STATIC_RUN(ActorCtrl) {
    rttr::registration::class_<ActorCtrl>("ActorCtrl")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
