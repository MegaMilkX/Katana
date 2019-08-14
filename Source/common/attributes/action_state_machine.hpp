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

    const char* getIconCode() const override { return ICON_MDI_RUN_FAST; }
};
REG_ATTRIB(ActionStateMachine, ActionStateMachine, Animation);

#endif
