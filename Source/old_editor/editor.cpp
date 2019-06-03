#include "editor.hpp"

#include "../common_old/scene_from_fbx.hpp"

#include "../common_old/lib/nativefiledialog/nfd.h"

#include "../common_old/lib/imguizmo/ImGuizmo.h"

#include "../common_old/util/has_suffix.hpp"

#include "../common_old/util/filesystem.hpp"

#include "../common_old/scene_components/scene_physics_world.hpp"

Editor::Editor()
: scene_tree(&object_inspector, &scene_window) {

}

void Editor::Init() {

    dir_view.init(get_module_dir() + "\\");

    scene = Scene::create();

    scene_tree.setScene(scene);

    scene_window.setScene(scene);

    //animator_sys.setScene(scene);

    input_lis = input().createListener();

    input_lis->bindActionPress(
        "ExitPlayMode",
        [this]() {
            play_mode = false;
            // for debugging
            // TODO: REMOVE
            //scene->copyFrom(game.getScene());
        }
    );

    game.init();
}

void Editor::Cleanup() {
    game.cleanup();

    input().removeListener(input_lis);

    scene->destroy();
}

void Editor::Update(GLFWwindow* window) {
    if(!play_mode) {
        _updateEditor(window);
    } else {
        _updatePlayMode(window);
    }
}

void Editor::AddWindow(EditorWindow* win) {
    windows.emplace_back(std::shared_ptr<EditorWindow>(win));
}

int Editor::SetupLayout() {
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

    ImGuiID dsid_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, NULL, &dockspace_id);
    ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.20f, NULL, &dockspace_id);
    ImGuiID dsid_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.30f, NULL, &dockspace_id);
    //ImGuiID dsid_down_right = ImGui::DockBuilderSplitNode(dsid_down, ImGuiDir_Right, 0.40f, NULL, &dsid_down);

    ImGui::DockBuilderDockWindow("Scene tree", dsid_left);
    ImGui::DockBuilderDockWindow("Object inspector", dsid_right);
    ImGui::DockBuilderDockWindow("Console", dsid_down);
    ImGui::DockBuilderDockWindow("DirView", dsid_down);
    ImGui::DockBuilderDockWindow("Scene", dockspace_id);

    ImGui::DockBuilderFinish(dockspace_id);
    return 0;
}

void Editor::_updateEditor(GLFWwindow* window) {
    double cursor_x, cursor_y;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);
    cursor_pos.x = (int)cursor_x;
    cursor_pos.y = (int)cursor_y;
    int w, h;
    glfwGetWindowSize(window, &w, &h);

    ImGuizmo::BeginFrame();

    double xcpos, ycpos;
    glfwGetCursorPos(window, &xcpos, &ycpos);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)xcpos, (float)ycpos);
    int mlstate, mrstate;
    mlstate = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    mrstate = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (mlstate == GLFW_PRESS) io.MouseDown[0] = true;
    else io.MouseDown[0] = false;
    if (mrstate == GLFW_PRESS) io.MouseDown[1] = true;
    else io.MouseDown[1] = false;

    //buildFrame();
    //animator_sys.Update(1.f/60.f);
    //

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
    /*
    ImGuiID dsid_left = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.20f, NULL, &dockspace_id);
    ImGuiID dsid_right = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.20f, NULL, &dockspace_id);
    ImGuiID dsid_down = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Down, 0.20f, NULL, &dockspace_id);
    */
    static int once = SetupLayout();

    bool temp_flag = true;
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New scene")) {
                //ImGui::OpenPopup("New scene?");
                editorState().selected_object = 0;
                scene->clear();
                LOG("Scene cleared");
            }
            if (ImGui::MenuItem("Open scene...")) {
                char* outPath;
                auto r = NFD_OpenDialog("scn", NULL, &outPath);
                if(r == NFD_OKAY) {
                    editorState().selected_object = 0;
                    scene->clear();

                    std::cout << outPath << std::endl;
                    std::string filePath(outPath);
                    scene->deserialize(filePath);
                    currentSceneFile = filePath;
                }
                else if(r == NFD_CANCEL) {
                    std::cout << "cancelled" << std::endl;
                }
                else {
                    std::cout << "error " << NFD_GetError() << std::endl;
                }
            }
            if (ImGui::MenuItem("Import from FBX...")) {
                char* outPath;
                auto r = NFD_OpenDialog("fbx", NULL, &outPath);
                if(r == NFD_OKAY) {
                    LOG(outPath);
                    std::string filePath(outPath);
                    editorState().selected_object = 0;
                    scene->clear();
                    sceneFromFbx(filePath, scene);
                }
            }
            if (ImGui::MenuItem("Merge...")) {

            }
            if (ImGui::MenuItem("Merge from FBX...")) {
                char* outPath;
                auto r = NFD_OpenDialog("fbx", NULL, &outPath);
                if(r == NFD_OKAY) {
                    LOG(outPath);
                    std::string filePath(outPath);
                    auto new_so = scene->createObject();
                    sceneFromFbx(filePath, scene, new_so);
                    editorState().selected_object = new_so;
                }
            }
            if (ImGui::MenuItem("Save")) {
                saveScene(scene);
            }
            if (ImGui::MenuItem("Save as...")) {
                saveScene(scene, true);
            }
            if (ImGui::MenuItem("Exit")) {

            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("New Viewport", "")) {
                
            }
            if (ImGui::MenuItem("Scene tree", "", temp_flag)) {

            }
            if (ImGui::MenuItem("Object inspector", "", temp_flag)) {

            }
            if (ImGui::MenuItem("Console", "", temp_flag)) {

            }
            if (ImGui::MenuItem("Scene tree", "", temp_flag)) {

            }
            if (ImGui::MenuItem("Material editor", "", temp_flag)) {

            }
            ImGui::EndMenu();
        }
        if(ImGui::MenuItem("Play...")) {
            play_mode = true;
            game.getScene()->clear();
            game.getScene()->copyFrom(scene);
        }

        ImGui::EndMenuBar();
    }
    bool sdw = true;
    ImGui::ShowDemoWindow(&sdw);
/*
    if(ImGui::BeginPopupModal("New scene?", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Clear all scene contents?");
        ImGui::Separator();
        if(ImGui::Button("OK", ImVec2(120, 0))) {
            editorState().selected_object = 0;
            scene->clear();
            LOG("Scene cleared");
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
*/

    DebugDraw::getInstance()->clear();
    // TODO: Update physics? 
    // Shouldn't be necessary for editor
    scene->getSceneComponent<PhysicsWorld>()->debugDraw();

    for(auto w : windows) {
        if(ImGui::Begin(MKSTR(w->Name() << "##" << w.get()).c_str(), &w->is_open)) {
            w->Update();
            ImGui::End();
        }
    }

    //ImGui::SetNextWindowDockID(dsid_left);
    if(ImGui::Begin("Scene tree")) {
        scene_tree.Update();
        ImGui::End();
    }
    //ImGui::SetNextWindowDockID(dsid_right);
    if(ImGui::Begin("Object inspector")) {
        object_inspector.Update();
        ImGui::End();
    }
    //ImGui::SetNextWindowDockID(dsid_down);
    if(ImGui::Begin("Console")) {
        console.Update();
        ImGui::End();
    }

    if(ImGui::Begin("DirView")) {
        dir_view.Update();
        ImGui::End();
    }

    scene_window.setCursorPos(cursor_pos);
    if(ImGui::Begin("Scene", 0, ImGuiWindowFlags_MenuBar)) {
        scene_window.Update();
        ImGui::End();
    }

    for(size_t i = 0; i < windows.size();) {
        if(!windows[i]->is_open) {
            windows.erase(windows.begin() + i);
            continue;
        }
        ++i;
    }

    ImGui::End();
}
void Editor::_updatePlayMode(GLFWwindow* window) {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    game.update(w, h);
}

bool Editor::saveScene(Scene* scene, bool forceDialog) {
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
            
            if(scene->serialize(filePath))
            {
                std::cout << "Scene saved" << std::endl;
                currentSceneFile = filePath;
            }
        }
    }
    else
    {
        if(scene->serialize(currentSceneFile))
        {
            std::cout << "Scene saved" << std::endl;
        }
    }
    
    return true;
}
