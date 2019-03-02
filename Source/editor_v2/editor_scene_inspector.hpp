#ifndef EDITOR_SCENE_INSPECTOR_HPP
#define EDITOR_SCENE_INSPECTOR_HPP

#include "scene/game_scene.hpp"

class Editor;
class EditorSceneInspector {
public:
    void update(Editor* editor);
private:
    void sceneTreeViewNode(GameObject* o, Editor* editor);
};

#endif
