#include "action_state_machine.hpp"

#include "../util/imgui_helpers.hpp"

void ActionStateMachine::update(float dt) {
    
}

void ActionStateMachine::onGui() {
    imguiResourceTreeCombo("action graph", graph, "action_graph", [](){

    });

    if(ImGui::ListBoxHeader("animations")) {
        ImGui::Selectable("Idle");
        ImGui::Selectable("Walk");
        ImGui::Selectable("Run");
        ImGui::Selectable("AddDetail");
        ImGui::Selectable("AddBreath");

        ImGui::ListBoxFooter();
    }
}