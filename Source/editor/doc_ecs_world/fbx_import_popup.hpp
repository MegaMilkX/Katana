#ifndef FBX_IMPORT_POPUP_HPP
#define FBX_IMPORT_POPUP_HPP


#include "../../common/util/imgui_helpers.hpp"
#include "state.hpp"

class GuiPopup {
public:
    virtual ~GuiPopup() {}
    virtual void onGui() = 0;
};

class GuiPopupFbxImport : public GuiPopup {
    DocEcsWorldState* state = 0;
    std::string path;
public:
    GuiPopupFbxImport(DocEcsWorldState* state, const char* path)
    : state(state), path(path) {}

    void onGui() override {
        if(ImGui::BeginPopup("FbxImport")) {
            if(ImGui::Button("Merge")) {
                
            }
            if(ImGui::Button("As SubScene")) {
                state->backupState();
                edTaskEcsWorldModelDragNDrop* task = new edTaskEcsWorldModelDragNDrop(
                    MKSTR("Importing " << node->getFullName()).c_str(),
                    node->getFullName().c_str(),
                    node->getName().c_str(),
                    state->world
                );
                state->model_dnd_tasks.insert(std::unique_ptr<edTaskEcsWorldModelDragNDrop>(task));
                edTaskPost(task);
            }
        }
        ImGui::EndPopup();
    }
};


#endif
