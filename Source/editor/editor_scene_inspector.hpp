#ifndef EDITOR_SCENE_INSPECTOR_HPP
#define EDITOR_SCENE_INSPECTOR_HPP

#include "../common/scene/game_scene.hpp"

class Editor;
class EditorSceneInspector {
public:
    EditorSceneInspector() {
        memset(search_string_buf, 0, sizeof(search_string_buf));
    }
    void update(Editor* editor);
private:
    void sceneTreeViewNode(GameObject* o, Editor* editor);

    char search_string_buf[256];
};

#endif
