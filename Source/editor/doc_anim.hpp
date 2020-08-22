#ifndef DOC_ANIM_HPP
#define DOC_ANIM_HPP

#include "editor_document.hpp"
#include "../common/resource/animation.hpp"
#include "../common/resource/skeleton.hpp"

#include "../common/ecs/world.hpp"
#include "../common/ecs/systems/scene_graph.hpp"
#include "../common/ecs/systems/render.hpp"
#include "../common/ecs/attribs/base_attribs.hpp"

#include "../common/gui_viewport.hpp"
#include "../common/util/imgui_ext.hpp"

class DocAnim : public EditorDocumentTyped<Animation> {
    ecsWorld world;
    ecsEntityHandle hRoot;
    ecsEntityHandle hLight;
    GuiViewport gvp;

    std::shared_ptr<ecsWorld> ref_object;
    std::shared_ptr<Skeleton> ref_skel;
    ecsEntityHandle cam_pivot;

    float cursor = .0f;
    bool playing = false;
    float playback_speed = 1.0f;

public:
    DocAnim();
    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;

    void onResourceSet() override;

};
STATIC_RUN(DocAnim) {
    regEditorDocument<DocAnim>("anm");
}

#endif
