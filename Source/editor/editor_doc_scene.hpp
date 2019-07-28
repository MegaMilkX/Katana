#ifndef EDITOR_DOC_SCENE_HPP
#define EDITOR_DOC_SCENE_HPP

#include "editor_document.hpp"

#include "../common/gui_viewport.hpp"

#include "editor_scene_inspector.hpp"
#include "editor_object_inspector.hpp"

#include "object_set.hpp"

#include "../common/input_listener.hpp"

class EditorDocScene : public EditorDocumentTyped<GameScene>, public InputListenerWrap {
    bool first_use = true;
    GuiViewport gvp;
    EditorSceneInspector scene_inspector;
    EditorObjectInspector object_inspector;
    std::shared_ptr<GameScene> scene;
    ObjectSet selected;
public:
    EditorDocScene();
    EditorDocScene(std::shared_ptr<ResourceNode>& node);

    virtual void onGui (Editor* ed);
};

#endif
