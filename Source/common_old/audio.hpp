#ifndef AUDIO_GLOBAL_HPP
#define AUDIO_GLOBAL_HPP

#include "util/audio/audio_mixer.hpp"

inline AudioMixer& audio() {
    static AudioMixer mixer;
    return mixer;
}

#endif
