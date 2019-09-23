#ifndef EDITOR_DOC_SCENE_HPP
#define EDITOR_DOC_SCENE_HPP

#include "editor_document.hpp"

#include "../common/gui_viewport.hpp"

#include "editor_scene_inspector.hpp"
#include "editor_object_inspector.hpp"

#include "../common/util/object_set.hpp"

#include "../common/input_listener.hpp"

#include "../katana/game_mode.hpp"

class EditorDocScene : public EditorDocumentTyped<GameScene>, public InputListenerWrap {
    bool first_use = true;
    GuiViewport gvp;
    EditorSceneInspector scene_inspector;
    EditorObjectInspector object_inspector;
    ObjectSet selected;

    GameScene* scn_ptr = 0;

    ktWorld world;
    bool running = false;

    rttr::type selected_game_mode = rttr::type::get<ktGameMode>();
public:
    EditorDocScene();

    void onFocus() override;

    virtual void onGui (Editor* ed, float dt);
    void onGuiToolbox(Editor* ed) override;
};
STATIC_RUN(EditorDocScene) {
    regEditorDocument<EditorDocScene>("so");
}

#endif
