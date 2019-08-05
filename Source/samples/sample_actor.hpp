#ifndef SAMPLE_ACTOR_HPP
#define SAMPLE_ACTOR_HPP

#include <common/attributes/actors/actor.hpp>
#include <common/world.hpp>

class actorSample : public ktActor {
    RTTR_ENABLE(ktActor)
public:
    void onStart(ktWorld* w) override {
        // TODO:
    }
    void onUpdate() override {
        // TODO:
    }
    void onCleanup() {
        // TODO:
    }
};
REG_ATTRIB(actorSample, SampleActor, Samples)

#endif
