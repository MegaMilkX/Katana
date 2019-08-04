#include "world.hpp"

#include "components/actors/actor.hpp"


void ktWorld::onAttribCreated(ktActor* a) {
    actors.insert(a);
    if(started) {
        a->onStart(this);
    }
}
void ktWorld::onAttribRemoved(ktActor* a) {
    actors.erase(a);
}


void ktWorld::start() {
    started = true;
    for(auto a : actors) {
        a->onStart(this);
    }
}
void ktWorld::update(float dt) {
    anim_ctrl->onUpdate();
    dynamics_ctrl->onUpdate();
    for(auto a : actors) {
        a->onUpdate();
    }
}
void ktWorld::cleanup() {
    started = false;
    for(auto a : actors) {
        a->onCleanup();
    }
}