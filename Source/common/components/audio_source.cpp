#include "audio_source.hpp"

AudioSource::~AudioSource() {
    audio().freeChannel(emid);
}

void AudioSource::onCreate() {}

STATIC_RUN(AudioSource) {
    rttr::registration::class_<AudioSource>("AudioSource")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}