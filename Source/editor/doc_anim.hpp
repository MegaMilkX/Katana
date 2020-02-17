#ifndef DOC_ANIM_HPP
#define DOC_ANIM_HPP

#include "editor_document.hpp"
#include "../common/resource/animation.hpp"
#include "../common/resource/skeleton.hpp"

#include "../common/scene/game_scene.hpp"
#include "../common/gui_viewport.hpp"
#include "../common/util/imgui_ext.hpp"

class DocAnim : public EditorDocumentTyped<Animation> {
    GameScene scn;
    GuiViewport gvp;

    std::shared_ptr<GameScene> ref_object;
    std::shared_ptr<Skeleton> ref_skel;
    ktNode* cam_pivot;

    float cursor = .0f;
    bool playing = false;
    float playback_speed = 1.0f;

public:
    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;

};
STATIC_RUN(DocAnim) {
    regEditorDocument<DocAnim>("anm");
}

#endif
