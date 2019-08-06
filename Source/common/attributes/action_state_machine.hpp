#ifndef ACTION_STATE_MACHINE_HPP
#define ACTION_STATE_MACHINE_HPP

#include "attribute.hpp"

class ActionStateMachine : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    void update(float dt);
};
REG_ATTRIB(ActionStateMachine, ActionStateMachine, Animation);

#endif
