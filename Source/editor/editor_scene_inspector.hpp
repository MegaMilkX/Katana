#ifndef EDITOR_SCENE_INSPECTOR_HPP
#define EDITOR_SCENE_INSPECTOR_HPP

#include "../common/scene/game_scene.hpp"

class Editor;
struct ObjectSet;
class EditorSceneInspector {
public:
    EditorSceneInspector() {
        memset(search_string_buf, 0, sizeof(search_string_buf));
    }
    void update(Editor* editor, const std::string& title = "Scene Inspector");
    void update(GameScene* scene, ObjectSet& selected, const std::string& title = "Scene Inspector");
private:
    void sceneTreeViewNode(ktNode* o, ObjectSet& selected);

    char search_string_buf[256];
};

#endif
