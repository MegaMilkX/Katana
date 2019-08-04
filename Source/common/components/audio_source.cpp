#include "audio_source.hpp"

AudioSource::~AudioSource() {
    audio().freeChannel(emid);
}

void AudioSource::onCreate() {}

REG_ATTRIB(AudioSource, AudioSource, Audio)