#include "action_state_machine.hpp"

#include "../util/imgui_helpers.hpp"

void ActionStateMachine::update(float dt) {
    
}

void ActionStateMachine::onGui() {
    imguiResourceTreeCombo("skeleton", skeleton, "skl", [](){

    });
    imguiResourceTreeCombo("graph", graph, "action_graph", [](){

    });

    if(ImGui::ListBoxHeader("animations")) {
        ImGui::Selectable("Idle");
        ImGui::Selectable("Walk");
        ImGui::Selectable("Run");
        ImGui::Selectable("AddDetail");
        ImGui::Selectable("AddBreath");

        ImGui::ListBoxFooter();
    }

    std::shared_ptr<Animation> anim;
    imguiResourceTreeCombo("anim", anim, "anm", [](){

    });
    char buf[256];
    memset(buf, 0, sizeof(buf));
    if(ImGui::InputText("alias", buf, sizeof(buf))) {

    }
}