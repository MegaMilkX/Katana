#ifndef VOLUME_TRIGGER_TEST_HPP
#define VOLUME_TRIGGER_TEST_HPP

#include "collision_listener.hpp"
#include "audio_source.hpp"
#include "light_source.hpp"
#include "../scene/controllers/dynamics_ctrl.hpp"
#include "../scene/hit_capsule.hpp"


class VolumeTriggerTest : public CollisionListener {
    RTTR_ENABLE(CollisionListener)
public:
    void onCreate() {}

    void onEnter(ktNode* other) {
        getOwner()->get<AudioSource>()->play(true);
        getOwner()->get<OmniLight>()->color = gfxm::vec3(0, 1.0f, .2f);
/*
        if(getOwner()->get_type() == rttr::type::get<HitCapsule>()) {
            ((HitCapsule*)getOwner())->onHit();
        }*/
        // TODO: ?
    }
    void onLeave(ktNode* other) {
        getOwner()->get<OmniLight>()->color = gfxm::vec3(1, .2f, .0f);
    }

    void write(SceneWriteCtx& out) override {

    }
    void read(SceneReadCtx& in) override {
        
    }
private:
};
REG_ATTRIB(VolumeTriggerTest, VolumeTriggerTest, Test);

#endif
