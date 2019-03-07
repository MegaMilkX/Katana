#include "editor.hpp"
#include <GLFW/glfw3.h>
#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"
#include "../common/lib/imguizmo/ImGuizmo.h"
#include "../common/util/log.hpp"

#include "../common/lib/nativefiledialog/nfd.h"

#include "../common/util/has_suffix.hpp"

#include "serialize_game_scene.hpp"

int setupImguiLayout();

Editor::Editor() {
    
}
Editor::~Editor() {
    
}

void Editor::init() {
    ImGuiInit();
    auto& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;

    input().getTable().addActionKey("MouseLeft", "MOUSE_LEFT");
    input().getTable().addActionKey("MouseRight", "MOUSE_RIGHT");
    input().getTable().addActionKey("MouseMiddle", "MOUSE_MIDDLE");

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
    input_lis->bindActionPress("MouseMiddle", [this](){

    });

    scene.reset(new GameScene());
    editor_scene.reset(new EditorScene(scene.get()));

    dir_view.init(get_module_dir());
    viewport.init(this, scene.get());

    LOG("Editor initialized");
}

void Editor::cleanup() {
    input().removeListener(input_lis);
    ImGuiCleanup();
}

void Editor::update(unsigned width, unsigned height, unsigned cursor_x, unsigned cursor_y) {
    ImGuiUpdate(1.0f/60.0f, width, height);
    ImGuizmo::BeginFrame();
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)cursor_x, (float)cursor_y);

    bool p_open = true;
    static bool opt_fullscreen_persistant = true;
    static ImGuiDockNodeFlags opt_flags = ImGuiDockNodeFlags_None;
    bool opt_fullscreen = opt_fullscreen_persistant;

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
        if(ImGui::BeginMenu("Scene")) {
            if(ImGui::MenuItem("Open")) {
                showOpenSceneDialog();
            }
            ImGui::Separator();
            if(ImGui::MenuItem("Save")) {
                showSaveSceneDialog(scene.get());
            }
            if(ImGui::MenuItem("Save As...")) {
                showSaveSceneDialog(scene.get(), true);
            }
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    //bool sdw = true;
    //ImGui::ShowDemoWindow(&sdw);

    viewport.update(this);
    scene_inspector.update(this);
    dir_view.update();
    object_inspector.update(this);
    // TODO: 

    ImGuiDraw();
}

GameScene* Editor::getScene() {
    return scene.get();
}
EditorScene& Editor::getEditorScene() {
    return *editor_scene.get();
}
GameObject* Editor::getSelectedObject() {
    return selected_object;
}
void Editor::setSelectedObject(GameObject* o) {
    selected_object = o;
}

bool Editor::showOpenSceneDialog() {
    char* outPath;
    auto r = NFD_OpenDialog("scn", NULL, &outPath);
    if(r == NFD_OKAY) {
        std::cout << outPath << std::endl;
        std::string filePath(outPath);
        setSelectedObject(0);
        getScene()->clear();
        deserializeScene(filePath, getScene());
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
            if(serializeScene(filePath, scene)) {
                LOG("Scene saved");
                currentSceneFile = filePath;
            } else {
                LOG_WARN("Failed to save scene");
            }
        }
    }
    else
    {
        if(serializeScene(currentSceneFile, scene)) {
            LOG("Scene saved");
        } else {
            LOG_WARN("Failed to save scene");
        }
    }
    
    return true;
}


int setupImguiLayout() {
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

    ImGuiID dsid_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, NULL, &dockspace_id);
    ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.20f, NULL, &dockspace_id);
    ImGuiID dsid_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.30f, NULL, &dockspace_id);
    //ImGuiID dsid_down_right = ImGui::DockBuilderSplitNode(dsid_down, ImGuiDir_Right, 0.40f, NULL, &dsid_down);

    ImGui::DockBuilderDockWindow("Scene Inspector", dsid_left);
    ImGui::DockBuilderDockWindow("Object inspector", dsid_right);
    ImGui::DockBuilderDockWindow("Console", dsid_down);
    ImGui::DockBuilderDockWindow("DirView", dsid_down);
    ImGui::DockBuilderDockWindow("Scene", dockspace_id);

    ImGui::DockBuilderFinish(dockspace_id);
    return 0;
}