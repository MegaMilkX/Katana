#ifndef ACTION_STATE_MACHINE_HPP
#define ACTION_STATE_MACHINE_HPP

#include "attribute.hpp"

#include "../resource/action_graph.hpp"

class ActionStateMachine : public Attribute {
    RTTR_ENABLE(Attribute)

    std::shared_ptr<ActionGraph> graph;
public:
    void update(float dt);

    void onGui() override;
};
REG_ATTRIB(ActionStateMachine, ActionStateMachine, Animation);

#endif
