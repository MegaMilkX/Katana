#ifndef MOTION_GUI_BLEND_TREE_HPP
#define MOTION_GUI_BLEND_TREE_HPP


#include "motion_gui.hpp"

#include "../../common/resource/blend_tree.hpp"
#include "../../common/resource/blend_tree_nodes.hpp"


class MotionGuiBlendTree : public MotionGuiBase {
    BlendTree* blendTree;

    JobGraphNode* selected_node = 0;

    void guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos);

public:
    MotionGuiBlendTree(const std::string& title, DocMotion* doc, BlendTree* blendTree);
    void drawGui(Editor* ed, float dt) override;
    void drawToolbox(Editor* ed) override;
};


#endif
