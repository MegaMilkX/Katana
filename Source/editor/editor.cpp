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

#include "editor_doc_scene.hpp"
#include "doc_motion.hpp"
#include "doc_material.hpp"
#include "doc_render_graph.hpp"
#include "doc_ecs_world.hpp"

int setupImguiLayout();

static void tryOpenDocument(const std::string& res_path) {
    if(!Editor::get()) {
        LOG_WARN("Editor is null");
        return;
    }
    Editor::get()->tryOpenDocument(res_path);
}

static void tryOpenNestedDocument(const std::string& res_path) {
    if(!Editor::get()) {
        LOG_WARN("Editor is null");
        return;
    }
    Editor::get()->tryOpenNestedDocument(res_path);
}

static void tryOpenDocumentFromPtr(std::shared_ptr<Resource> res) {
    if(!Editor::get()) {
        LOG_WARN("Editor is null");
        return;
    }
    Editor::get()->tryOpenDocumentFromPtr(res);
}


Editor::Editor() {
    gTryOpenDocumentFn = &::tryOpenNestedDocument;
    gTryOpenDocumentFromPtrFn = &::tryOpenDocumentFromPtr;
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
    input().getTable().addActionKey("ALT", "KB_LEFT_ALT");
    input().getTable().addActionKey("Z", "KB_Z");
    input().getTable().addActionKey("S", "KB_S");

    input().getTable().addActionKey("F1", "KB_F1");
    input().getTable().addActionKey("F2", "KB_F2");
    input().getTable().addActionKey("F3", "KB_F3");
    input().getTable().addActionKey("F4", "KB_F4");
    input().getTable().addActionKey("F5", "KB_F5");
    input().getTable().addActionKey("F6", "KB_F6");
    input().getTable().addActionKey("F7", "KB_F7");

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
        //if(ctrl && !shift) undo();
        //else if(ctrl && shift) redo();
    });
    input_lis->bindActionPress("S", [this](){
        if(focused_document && ctrl) {
            focused_document->save();
        }
    });

    input_lis->bindActionPress("F1", [](){ dbg_renderBufferId = 0; });
    input_lis->bindActionPress("F2", [](){ dbg_renderBufferId = 1; });
    input_lis->bindActionPress("F3", [](){ dbg_renderBufferId = 2; });
    input_lis->bindActionPress("F4", [](){ dbg_renderBufferId = 3; });
    input_lis->bindActionPress("F5", [](){ dbg_renderBufferId = 4; });
    input_lis->bindActionPress("F6", [](){ dbg_renderBufferId = 5; });
    input_lis->bindActionPress("F7", [](){ dbg_renderBufferId = 6; });

    LOG("Editor initialized");
}

void Editor::onCleanup() {

    input().removeListener(input_lis);
}

void Editor::onUpdate() {
    
}

void Editor::onGui(float dt) {
    platformMouseSetEnabled(true);
    
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
    //if (opt_flags & ImGuiDockNodeFlags_PassthruDockspace)
    //    window_flags |= ImGuiWindowFlags_NoBackground;

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
                    addWindow(new EditorDocScene());
                }
                if(ImGui::MenuItem("Motion")) {
                    addWindow(new DocMotion());
                }
                if(ImGui::MenuItem("Material")) {
                    addWindow(new DocMaterial());
                }
                if(ImGui::MenuItem("RenderGraph")) {
                    addWindow(new DocRenderGraph());
                }
                if(ImGui::MenuItem("EcsWorld")) {
                    addWindow(new DocEcsWorld());
                }
                if(ImGui::MenuItem("Texture2D", 0, false, false)) {}
                if(ImGui::MenuItem("Mesh", 0, false, false)) {}
                if(ImGui::MenuItem("Shader", 0, false, false)) {}
                if(ImGui::MenuItem("AudioClip", 0, false, false)) {}
                ImGui::EndMenu();
            }
            if(ImGui::MenuItem("Save")) {
                if(focused_document) {
                    focused_document->save();
                }
            }
            if(ImGui::MenuItem("Save as...")) {
                if(focused_document) {
                    // TODO
                }
            }
            if(ImGui::MenuItem("Exit")) {}
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("Configuration")) {
            if(ImGui::MenuItem("Settings")) {
                
            }
            if(ImGui::MenuItem("Color scheme")) {

            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    EditorWindow* doc_to_be_closed = 0;

    ed_resource_tree.update(this);
    for(auto& d : open_documents) {
        if(!d->isOpen()) {
            if(d->isUnsaved()) {
                ImGui::OpenPopup("Unsaved document");
                doc_to_be_closed = d;
                break;
            } else {
                delete d;
                open_documents.erase(d);
                setFocusedWindow(0);
            }
        }
    }

    if(ImGui::BeginPopupModal("Unsaved document")) {
        ImGui::Text(MKSTR("Close without saving?").c_str());
        if(ImGui::Button("Close")) {
            delete doc_to_be_closed;
            open_documents.erase(doc_to_be_closed);
            setFocusedWindow(0);
            ImGui::CloseCurrentPopup();
        } ImGui::SameLine();
        if(ImGui::Button("Cancel")) {
            doc_to_be_closed->setOpen(true);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    for(auto d : open_documents) {
        d->drawAsRoot(this, dt);
    }
    if(ImGui::Begin("Toolbox")) {
        if(focused_document) {
            focused_document->drawToolbox(this);
        }
    }
    ImGui::End();

    ImGui::End();

    ImGui::PushID(ImGui::GetID("PopupHint1"));
    bool dummy = true;
    if(ImGui::BeginPopupModal("Hint1", &dummy)) {
        ImGui::Text(MKSTR("No editor or viewer exist for this resource type").c_str());
        if(ImGui::Button("OK")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();

    //bool demo = true;
    //ImGui::ShowDemoWindow(&demo);
}

EditorWindow* Editor::getFocusedWindow() {
    return focused_document;
}

EditorResourceTree& Editor::getResourceTree() {
    return ed_resource_tree;
}

void Editor::setCurrentDockspace(ImGuiID id) {
    current_dockspace = id;
}

void Editor::setFocusedWindow(const std::string& name) {
    for(auto w : open_documents) {
        if(w->getName() == name) {
            setFocusedWindow(w);
        }
    }
}
void Editor::setFocusedWindow(EditorWindow* w) {
    if(focused_document != w) {
        if(focused_document) focused_document->onUnfocus();
        if(w) w->onFocus();
    }
    focused_document = w;
}

void Editor::addWindow(EditorWindow* doc) {
    open_documents.insert(doc);
    ImGui::SetWindowFocus(doc->getTitle().c_str());
    ImGui::DockBuilderDockWindow(doc->getTitle().c_str(), current_dockspace);
    setFocusedWindow(doc);
}
void Editor::tryOpenDocument(const std::string& res_path) {
    EditorWindow* doc = 0;
    for(auto &d : open_documents) {
        // TODO: 
        if(d->getName() == res_path) {
            doc = d;
            break;
        }
    }

    if(doc) {
        // TODO: getWindowName()
        ImGui::SetWindowFocus(doc->getTitle().c_str());
        setFocusedWindow(doc);
        return;
    }

    doc = createEditorDocument(res_path);
    if(!doc) {
        ImGui::PushID(ImGui::GetID("PopupHint1"));
        ImGui::OpenPopup("Hint1");
        ImGui::PopID();
        return;
    }
    addWindow(doc);
}

void Editor::tryOpenNestedDocument(const std::string& res_path) {
    EditorWindow* doc = 0;
    EditorWindow* current_doc = getFocusedWindow();
    if(!current_doc) {
        LOG_WARN("No window in focus");
        return;
    }

    doc = createEditorDocument(res_path);
    current_doc->setNestedWindow(doc);
}

void Editor::tryOpenDocumentFromPtr(std::shared_ptr<Resource> res) {
    if(!res) return;
    rttr::type t = res->get_type();

    EditorWindow* current_doc = getFocusedWindow();
    if(!current_doc) {
        LOG_WARN("No window in focus");
        return;
    }

    EditorWindow* doc = createEditorDocument(t);
    current_doc->setNestedWindow(doc);
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
        //setSelectedObject(0);
        //getScene()->clear();
        //getScene()->read(filePath);
        currentSceneFile = filePath;
    }
    else if(r == NFD_CANCEL) {
        std::cout << "cancelled" << std::endl;
        return false;
    }
    else {
        std::cout << "error " << NFD_GetError() << std::endl;
        return false;
    }
    return true;
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
            
        }
    }
    else
    {
        
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
    ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.20f, NULL, &dockspace_id);
    ImGuiID dsid_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.40f, NULL, &dockspace_id);

    ImGui::DockBuilderDockWindow("Resource Tree", dsid_left);
    ImGui::DockBuilderDockWindow("Directory", dsid_down);
    ImGui::DockBuilderDockWindow("Toolbox", dsid_right);

    ImGui::DockBuilderFinish(dockspace_id);
    current_dockspace = dockspace_id;

    return 0;
}

