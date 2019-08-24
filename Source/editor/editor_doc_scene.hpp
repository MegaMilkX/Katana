#ifndef EDITOR_DOC_SCENE_HPP
#define EDITOR_DOC_SCENE_HPP

#include "editor_document.hpp"

#include "../common/gui_viewport.hpp"

#include "editor_scene_inspector.hpp"
#include "editor_object_inspector.hpp"

#include "object_set.hpp"

#include "../common/input_listener.hpp"

#include "../katana/game_mode.hpp"

class EditorDocScene : public EditorDocumentTyped<GameScene>, public InputListenerWrap {
    bool first_use = true;
    GuiViewport gvp;
    EditorSceneInspector scene_inspector;
    EditorObjectInspector object_inspector;
    ObjectSet selected;

    rttr::type selected_game_mode = rttr::type::get<ktGameMode>();
public:
    EditorDocScene();
    EditorDocScene(std::shared_ptr<ResourceNode>& node);

    void onFocus() override;

    virtual void onGui (Editor* ed);
    void onGuiToolbox(Editor* ed) override;
};

#endif
