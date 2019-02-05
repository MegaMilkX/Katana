#ifndef EDITOR_H
#define EDITOR_H

#include <vector>
#include <memory>

#include <glfw/glfw3.h>

#include "resource/resource_factory.h"
#include "resource/texture2d.h"

#include "editor_window.hpp"
#include "editor_viewport.hpp"
#include "editor_scene.hpp"
#include "editor_scene_tree.hpp"
#include "editor_object_inspector.hpp"
#include "editor_console.hpp"
#include "editor_dir_view.hpp"

#include "scene.hpp"

#include "input/input_mgr.hpp"

#include "animator_sys.hpp"
#include "systems/physics_system.hpp"

#include "game.hpp"

class Editor {
public:
    Editor();
    
    void Init();
    void Cleanup();

    void Update(GLFWwindow* window);

    void AddWindow(EditorWindow* win);
private:
    int SetupLayout();

    void _updateEditor(GLFWwindow* window);
    void _updatePlayMode(GLFWwindow* window);

    bool saveScene(Scene* scene, bool forceDialog = false);

    InputListener* input_lis;

    bool play_mode = false;

    std::vector<std::shared_ptr<EditorWindow>> windows;

    EditorSceneTree scene_tree;
    EditorObjectInspector object_inspector;
    EditorConsole console;
    EditorDirView dir_view;
    EditorScene scene_window;

    Scene* scene;
    std::string currentSceneFile;
    Game game;
    AnimatorSys animator_sys;

    gfxm::ivec2 cursor_pos;
};

#endif
