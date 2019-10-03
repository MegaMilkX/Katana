#include "audio_source.hpp"

AudioSource::~AudioSource() {
    audio().freeChannel(emid);
}

void AudioSource::onCreate() {}