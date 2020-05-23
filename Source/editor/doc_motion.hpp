#ifndef DOC_MOTION_HPP
#define DOC_MOTION_HPP

#include "editor_document.hpp"

#include "motion_gui/motion_gui_fsm.hpp"
#include "motion_gui/motion_gui_blend_tree.hpp"
#include "../common/ecs/world.hpp"

class DocMotion : public EditorDocumentTyped<Motion> {
    std::vector<std::unique_ptr<MotionGuiBase>> gui_stack;

    //AnimSampleBuffer sample_buf;

    // Preview stuff
    GuiViewport                         viewport;
    ecsWorld                            world;
    std::shared_ptr<EntityTemplate>     reference;

    //ktNode* cam_pivot = 0;
    //DirLight* cam_light = 0;
    //ktNode* root_motion_node = 0;
    gfxm::vec3 root_motion_pos;
    gfxm::quat root_motion_rot;
    int        root_motion_bone_id = -1;
    // ========

    void resetGui();
    void setReferenceObject(std::shared_ptr<EntityTemplate> tpl);

public:
    DocMotion();

    void pushGuiLayer(MotionGuiBase* gui);

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;

    void onResourceSet() override;
};
STATIC_RUN(DocMotion) {
    regEditorDocument<DocMotion>({ "motion" });
}

#endif
