#ifndef AUDIO_LISTENER_HPP
#define AUDIO_LISTENER_HPP

#include "component.hpp"
#include "../../common/gfxm.hpp"
#include "../scene/node.hpp"

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

    virtual const char* getIconCode() const { return ICON_MDI_MICROPHONE; }
private:

};
REG_ATTRIB(AudioListener, AudioListener, Audio);

#endif
