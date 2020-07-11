#include "imgui_helpers.hpp"

#include "ecs/world.hpp"

tryOpenDocumentFn_t gTryOpenDocumentFn = 0;
tryOpenDocumentFromPtrFn_t gTryOpenDocumentFromPtrFn = 0;


bool imguiEntityCombo(const char* label, ecsWorld* world, ecsEntityHandle& out) {
    bool ret = false;
    if(ImGui::BeginCombo(label, "Select entity...")) {
        

        ImGui::EndCombo();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ENTITY")) {
            ecsEntityHandle hdlp = *(ecsEntityHandle*)payload->Data;
            out = hdlp;
            ret = true;
        }
        ImGui::EndDragDropTarget();
    }

    return ret;
}