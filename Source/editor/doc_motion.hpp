#ifndef DOC_MOTION_HPP
#define DOC_MOTION_HPP

#include "editor_document.hpp"

#include "../common/resource/motion.hpp"
#include "../common/resource/anim_fsm.hpp"
#include "../common/resource/blend_tree.hpp"

class MotionGuiBase {
public:
    virtual void drawGui(Editor* ed, float dt) = 0;
    virtual void drawToolbox(Editor* ed) = 0;
};

class MotionGuiFSM : public MotionGuiBase {
    AnimFSM* fsm;

public:
    MotionGuiFSM(AnimFSM* fsm)
    : fsm(fsm) {}
    void drawGui(Editor* ed, float dt) override;
    void drawToolbox(Editor* ed) override;
};

class MotionGuiBlendTree : public MotionGuiBase {
    BlendTree* blendTree;

public:
    MotionGuiBlendTree(BlendTree* blendTree)
    : blendTree(blendTree) {}
    void drawGui(Editor* ed, float dt) override;
    void drawToolbox(Editor* ed) override;
};

class DocMotion : public EditorDocumentTyped<Motion> {
    std::unique_ptr<MotionGuiBase> gui;

    void resetGui();

public:
    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;
};
STATIC_RUN(DocMotion) {
    regEditorDocument<DocMotion>({ "motion" });
}

#endif
