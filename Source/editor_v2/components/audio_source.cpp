#include "audio_source.hpp"

#include "../scene/controllers/audio_controller.hpp"

AudioSource::~AudioSource() {
    audio().freeChannel(emid);
}

void AudioSource::onCreate() {
    getOwner()->getScene()->getController<AudioController>();
}

STATIC_RUN(AudioSource) {
    rttr::registration::class_<AudioSource>("AudioSource")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}