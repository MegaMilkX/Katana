#ifndef AUDIO_LISTENER_HPP
#define AUDIO_LISTENER_HPP

#include "component.hpp"
#include "../../common/gfxm.hpp"
#include "../scene/game_object.hpp"

#include "../../common/audio.hpp"

class AudioListener : public Attribute {
    RTTR_ENABLE(Attribute)
public:


    virtual bool serialize(out_stream& out) {
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        return true;
    }
private:

};
STATIC_RUN(AudioListener) {
    rttr::registration::class_<AudioListener>("AudioListener")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
