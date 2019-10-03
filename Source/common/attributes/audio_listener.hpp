#ifndef AUDIO_LISTENER_HPP
#define AUDIO_LISTENER_HPP

#include "attribute.hpp"
#include "../../common/gfxm.hpp"
#include "../scene/node.hpp"

#include "../../common/audio.hpp"

class AudioListener : public Attribute {
    RTTR_ENABLE(Attribute)
public:

    virtual const char* getIconCode() const { return ICON_MDI_MICROPHONE; }
private:

};

#endif
