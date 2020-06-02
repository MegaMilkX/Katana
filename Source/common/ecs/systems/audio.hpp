#ifndef ECS_AUDIO_SYSTEM_HPP
#define ECS_AUDIO_SYSTEM_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

#include "../../audio.hpp"

class ecsTupleAudioSource : public ecsTuple<
    ecsWorldTransform,
    ecsAudioSource
> {
public:
    size_t channel = -1;
};
class ecsTupleAudioListener : public ecsTuple<
    ecsWorldTransform,
    ecsAudioListener
> {
public:

};

class ecsAudioSys : public ecsSystem<
    ecsTupleAudioSource,
    ecsTupleAudioListener
> {
public:
    void onFit(ecsTupleAudioSource* src) override {
        src->channel = audio().createChannel();
        ecsAudioSource* s = src->get<ecsAudioSource>();
        audio().setLooping(src->channel, s->isLooping());
        if(s->getClip()) {
            audio().setBuffer(src->channel, s->getClip()->getBuffer());
        }
        gfxm::vec3 pos = src->get<ecsWorldTransform>()->transform * gfxm::vec4(0,0,0,1);
        audio().setPosition(src->channel, pos);
        if(s->isAutoplay()) {
            audio().play3d(src->channel);
        }
    }
    void onUnfit(ecsTupleAudioSource* src) override {
        audio().freeChannel(src->channel);
    }

    void onUpdate() override {
        for(auto& a : get_array<ecsTupleAudioListener>()) {
            audio().setListenerTransform(a->get<ecsWorldTransform>()->transform);
            // TODO: Only one listener is allowed, no need for array
            // rethink this
            break;
        }
        for(auto& a : get_array<ecsTupleAudioSource>()) {
            // TODO: Optimize, no need to move static sources
            gfxm::vec3 pos = a->get<ecsWorldTransform>()->transform * gfxm::vec4(0,0,0,1);
            audio().setPosition(a->channel, pos);
            ecsAudioSource::CMD cmd = a->get<ecsAudioSource>()->_getCmd();
            a->get<ecsAudioSource>()->_setPlaying(audio().isPlaying(a->channel));
            if(cmd == ecsAudioSource::PLAY) {
                audio().play3d(a->channel);
                a->get<ecsAudioSource>()->_setPlaying(true);
            } else if(cmd == ecsAudioSource::PAUSE) {
                audio().stop(a->channel);
                a->get<ecsAudioSource>()->_setPlaying(false);
            } else if(cmd == ecsAudioSource::STOP) {
                audio().stop(a->channel);
                audio().resetCursor(a->channel);
                a->get<ecsAudioSource>()->_setPlaying(false);
            }
            a->get<ecsAudioSource>()->_clearCmd();
        }
    }
};


#endif
