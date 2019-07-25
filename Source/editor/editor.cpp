#include "editor.hpp"
#include <GLFW/glfw3.h>
#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/lib/imguizmo/ImGuizmo.h"
#include "../common/util/log.hpp"

#include "../common/lib/nativefiledialog/nfd.h"

#include "../common/util/has_suffix.hpp"

#include "../common/audio.hpp"

#include "../common/util/system.hpp"

#include "../common/scene/controllers/render_controller.hpp"
#include "../common/scene/controllers/constraint_ctrl.hpp"
#include "../common/scene/controllers/anim_controller.hpp"

#include "../common/platform/platform.hpp"

#include "../common/resource/resource_desc_library.hpp"

int setupImguiLayout();

Editor::Editor() {
    
}
Editor::~Editor() {
    
}

void Editor::onInit() {
    input().getTable().addActionKey("MouseLeft", "MOUSE_LEFT");
    input().getTable().addActionKey("MouseRight", "MOUSE_RIGHT");
    input().getTable().addActionKey("MouseMiddle", "MOUSE_MIDDLE");
    input().getTable().addActionKey("ExitPlayMode", "KB_ESCAPE");
    input().getTable().addActionKey("GizmoT", "KB_W");
    input().getTable().addActionKey("GizmoR", "KB_E");
    input().getTable().addActionKey("GizmoS", "KB_R");
    input().getTable().addActionKey("DebugDrawToggle", "KB_Q");
    input().getTable().addActionKey("CTRL", "KB_LEFT_CONTROL");
    input().getTable().addActionKey("SHIFT", "KB_LEFT_SHIFT");
    input().getTable().addActionKey("Z", "KB_Z");

    input_lis = input().createListener();
    input_lis->bindActionPress("MouseLeft", [this](){
        ImGui::GetIO().MouseDown[0] = true;
    });
    input_lis->bindActionRelease("MouseLeft", [this](){
        ImGui::GetIO().MouseDown[0] = false;
    });
    input_lis->bindActionPress("MouseRight", [this](){
        ImGui::GetIO().MouseDown[1] = true;
    });
    input_lis->bindActionRelease("MouseRight", [this](){
        ImGui::GetIO().MouseDown[1] = false;
    });
    input_lis->bindActionPress("ExitPlayMode", [this](){
    });
    input_lis->bindActionPress("GizmoT", [this](){
        if(ImGui::GetIO().WantTextInput) return;
        editor_state.tgizmo_mode = TGIZMO_T;
    });
    input_lis->bindActionPress("GizmoR", [this](){
        if(ImGui::GetIO().WantTextInput) return;
        editor_state.tgizmo_mode = TGIZMO_R;
    });
    input_lis->bindActionPress("GizmoS", [this](){
        if(ImGui::GetIO().WantTextInput) return;
        editor_state.tgizmo_mode = TGIZMO_S;
    });
    input_lis->bindActionPress("DebugDrawToggle", [this](){
        if(ImGui::GetIO().WantTextInput) return;
        editor_state.debug_draw = !editor_state.debug_draw;
    });
    input_lis->bindActionPress("CTRL", [this](){
        ctrl = true;
    });
    input_lis->bindActionRelease("CTRL", [this](){
        ctrl = false;
    });
    input_lis->bindActionPress("SHIFT", [this](){
        shift = true;
    });
    input_lis->bindActionRelease("SHIFT", [this](){
        shift = false;
    });
    input_lis->bindActionPress("Z", [this](){
        if(ImGui::GetIO().WantTextInput) return;
        if(ctrl && !shift) undo();
        else if(ctrl && shift) redo();
        else viewport.recenterCamera();
    });

    scene.reset(new GameScene());
    scene->getController<RenderController>();
    scene->getController<ConstraintCtrl>();
    scene->getController<AnimController>();
    scene->startSession();

    dir_view.init(get_module_dir() + "/" + platformGetConfig().data_dir);
    viewport.init(this, scene.get());

    LOG("Editor initialized");
}

void Editor::onCleanup() {
    scene->stopSession();

    input().removeListener(input_lis);
}

void Editor::onUpdate() {
    
}

void Editor::onGui() {
    ImGuizmo::BeginFrame();

    bool p_open = true;
    static bool opt_fullscreen_persistent = true;
    static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
    bool opt_fullscreen = opt_fullscreen_persistent;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }

    // When using ImGuiDockNodeFlags_PassthruDockspace, DockSpace() will render our background and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &p_open, window_flags);
    ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    ImGui::DockSpace(dockspace_id);

    static int once = setupImguiLayout();

    if(ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            if(ImGui::BeginMenu("New Resource")) {
                if(ImGui::MenuItem("Scene")) {
                    addNewDocument(new EditorDocScene(0));
                }
                if(ImGui::MenuItem("Texture2D")) {}
                if(ImGui::MenuItem("Material")) {}
                if(ImGui::MenuItem("Mesh")) {}
                if(ImGui::MenuItem("Shader")) {}
                if(ImGui::MenuItem("AudioClip")) {}
                ImGui::EndMenu();
            }
            if(ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }
        /*
        if(ImGui::BeginMenu("Scene")) {
            if(ImGui::MenuItem("New")) {
                scene->clear();
                currentSceneFile = "";
                history.clear();
                backupScene("new scene");
                selected_objects.clear();
            }
            if(ImGui::MenuItem("Open")) {
                if(showOpenSceneDialog()) {
                    history.clear();
                    backupScene("scene loaded");
                }
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Save")) {
                showSaveSceneDialog(scene.get());
            }
            if(ImGui::MenuItem("Save As...")) {
                showSaveSceneDialog(scene.get(), true);
            }
            ImGui::EndMenu();
        } */
        /*
        if(ImGui::MenuItem("Play mode")) {
            std::string play_cache_path = get_module_dir() + "\\play_cache.scn";
            //serializeScene(play_cache_path, getScene());
            create_process(get_module_path(), "play \"" + play_cache_path + "\"");
            /*
            editorState().is_play_mode = true;
            editorState().game_state->getScene()->clear();
            editorState().game_state->getScene()->copy(getScene());
            editorState().game_state->getScene()->startSession();
        } */
        ImGui::EndMenuBar();
    }

    ed_resource_tree.update(this);
    for(auto& kv : documents) {
        auto doc = kv.second;
        if(!doc->isOpen()) {
            delete doc;
            documents.erase(kv.first);
        }
    }
    for(auto& d : new_documents) {
        if(!d->isOpen()) {
            delete d;
            new_documents.erase(d);
        }
    }
    
    if(documents.empty()) {
        // TODO
        //setCurrentDockspace(dockspace_id);
    }
    for(auto& kv : documents) {
        kv.second->update(this);
    }
    for(auto d : new_documents) {
        d->update(this);
    }

    //viewport.update(this);
    //scene_inspector.update(this);
    //dir_view.update(this);
    //object_inspector.update(this);
    //asset_inspector.update(this);

    /*
    if(ImGui::Begin("Toolbox")) {
        ImGui::Separator();
        bool dd = viewport.getViewport().debugDrawEnabled();
        ImGui::Checkbox("Debug draw", &dd);
        viewport.getViewport().enableDebugDraw(dd);

        ImGui::End();
    }
    if(ImGui::Begin("History", &history_open, ImVec2(100, 200))) {
        history.onGui();

        ImGui::End();
    }
    if(ImGui::Begin("Session")) {
        if(ImGui::BeginCombo("session", "<null>")) {
            
            ImGui::EndCombo();
        }
        ImGui::End();
    } */
    // TODO: 

    ImGui::End();

    bool dummy = true;
    if(ImGui::BeginPopupModal("Hint1", &dummy)) {
        ImGui::Text(MKSTR("No editor or viewer exist for this resource type").c_str());
        if(ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    if(ImGui::BeginPopupModal("Hint2", &dummy)) {
        ImGui::Text(MKSTR("Can't deduce resource type without extension").c_str());
        if(ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }    
}

GameScene* Editor::getScene() {
    return scene.get();
}
ObjectSet& Editor::getSelectedObjects() {
    return selected_objects;
}
EditorAssetInspector* Editor::getAssetInspector() {
    return &asset_inspector;
}
EditorResourceTree& Editor::getResourceTree() {
    return ed_resource_tree;
}

void Editor::setCurrentDockspace(ImGuiID id) {
    current_dockspace = id;
}

void Editor::setSelectedObject(GameObject* o) {
    selected_objects.clearAndAdd(o);
}

void Editor::addDocument(const ResourceNode* node, EditorDocument* doc) {
    documents[node] = doc;
    ImGui::SetWindowFocus(doc->getName().c_str());
    ImGui::DockBuilderDockWindow(doc->getName().c_str(), current_dockspace);
}
void Editor::addNewDocument(EditorDocument* doc) {
    new_documents.insert(doc);
    ImGui::SetWindowFocus(doc->getName().c_str());
    ImGui::DockBuilderDockWindow(doc->getName().c_str(), current_dockspace);
}
void Editor::tryOpenDocument(const ResourceNode* node) {
    auto it = documents.find(node);
    if(it != documents.end()) {
        ImGui::SetWindowFocus(it->second->getName().c_str());
        return;
    }
    std::string node_name = node->getName();
    size_t dot_pos = node_name.find_last_of(".");
    if(dot_pos != node_name.npos) {
        std::string ext = node_name.substr(dot_pos + 1);
        ResourceDesc* desc = ResourceDescLibrary::get()->find(ext);
        if(!desc) {
            ImGui::OpenPopup("Hint1");
        } else {
            addDocument(node, desc->create_resource_doc_fn((ResourceNode*)node));
        }
    } else {
        ImGui::OpenPopup("Hint2");
    }
}

void Editor::backupScene(const std::string& label) {
    history.push(scene.get(), label);
}
void Editor::redo() {
    LOG("REDO");
    setSelectedObject(0);
    scene->stopSession();
    history.redo(scene.get());
    //history.move_forward();
    //if(!history.current()) return;
    //scene->copy(history.current());
}
void Editor::undo() {
    LOG("UNDO");
    setSelectedObject(0);
    scene->stopSession();
    history.undo(scene.get());
    //history.move_back();
    //if(!history.current()) return;
    //scene->copy(history.current());
}

EditorState& Editor::getState() {
    return editor_state;
}

bool Editor::showOpenSceneDialog() {
    char* outPath;
    auto r = NFD_OpenDialog("scn", NULL, &outPath);
    if(r == NFD_OKAY) {
        std::cout << outPath << std::endl;
        std::string filePath(outPath);
        setSelectedObject(0);
        getScene()->clear();
        getScene()->read(filePath);
        currentSceneFile = filePath;
    }
    else if(r == NFD_CANCEL) {
        std::cout << "cancelled" << std::endl;
    }
    else {
        std::cout << "error " << NFD_GetError() << std::endl;
    }
}

bool Editor::showSaveSceneDialog(GameScene* scene, bool forceDialog) {
    if(currentSceneFile.empty() || forceDialog)
    {
        char* outPath;
        auto r = NFD_SaveDialog("scn", NULL, &outPath);
        if(r == NFD_OKAY) {
            std::string filePath(outPath);
            if(!has_suffix(filePath, ".scn")) {
                filePath = filePath + ".scn";
            }
            std::cout << filePath << std::endl;
            if(getScene()->write(filePath)) {
                LOG("Scene saved");
                currentSceneFile = filePath;
            } else {
                LOG_WARN("Failed to save scene");
            }
        }
    }
    else
    {
        if(getScene()->write(currentSceneFile)) {
            LOG("Scene saved");
        } else {
            LOG_WARN("Failed to save scene");
        }
    }
    
    return true;
}


int Editor::setupImguiLayout() {
    dockspace_id = ImGui::GetID("MyDockspace");
    /*
    ImGuiID dsid_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, NULL, &dockspace_id);
    ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.20f, NULL, &dockspace_id);
    ImGuiID dsid_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.30f, NULL, &dockspace_id);
    ImGuiID dsid_center_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.15f, NULL, &dockspace_id);
    ImGuiID dsid_down_left = ImGui::DockBuilderSplitNode(dsid_left, ImGuiDir_Down, 0.45f, NULL, &dsid_left);
    ImGuiID dsid_down_right = ImGui::DockBuilderSplitNode(dsid_right, ImGuiDir_Down, 0.25f, NULL, &dsid_right);

    ImGui::DockBuilderDockWindow("Toolbox", dsid_center_right);
    ImGui::DockBuilderDockWindow("Scene Inspector", dsid_left);
    ImGui::DockBuilderDockWindow("Object inspector", dsid_right);
    ImGui::DockBuilderDockWindow("Resource inspector", dsid_right);
    ImGui::DockBuilderDockWindow("Console", dsid_down);
    ImGui::DockBuilderDockWindow("DirView", dsid_down);
    ImGui::DockBuilderDockWindow("Scene", dockspace_id);
    ImGui::DockBuilderDockWindow("Session", dsid_down_left);
    ImGui::DockBuilderDockWindow("History", dsid_down_right);
     */

    ImGuiID dsid_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, NULL, &dockspace_id);

    ImGui::DockBuilderDockWindow("Resource Tree", dsid_left);

    ImGui::DockBuilderFinish(dockspace_id);
    current_dockspace = dockspace_id;
    return 0;
}

