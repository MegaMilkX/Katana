#include "doc_motion.hpp"

void DocMotion::onGui(Editor* ed, float dt) {
    ImGui::Text("Create new");
    ImGui::Button("Animation FSM");
    ImGui::Text("OR");
    ImGui::Button("BlendTree");
}

void DocMotion::onGuiToolbox(Editor* ed) {

}