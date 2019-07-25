#ifndef EDITOR_OBJECT_INSPECTOR_HPP
#define EDITOR_OBJECT_INSPECTOR_HPP

#include <string>

class Editor;
class GameScene;
struct ObjectSet;
class EditorObjectInspector {
public:
    void update(Editor* editor, const std::string& title = "Object Inspector");
    void update(GameScene* scene, ObjectSet& selected, const std::string& title = "Object Inspector");
};

#endif
