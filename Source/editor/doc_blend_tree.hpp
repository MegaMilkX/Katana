#ifndef DOC_BLEND_TREE_HPP
#define DOC_BLEND_TREE_HPP

#include "editor_document.hpp"
#include "../common/resource/blend_tree.hpp"
#include "../common/resource/resource_tree.hpp"

#include "../common//scene/game_scene.hpp"
#include "../common/gui_viewport.hpp"

#include "../common/resource/skeleton.hpp"
#include "../common/resource/animation.hpp"

#include "../common/util/func_graph/node_graph.hpp"

#include "../common/util/imgui_helpers.hpp"

#include "../common/attributes/light_source.hpp"

#include "../common/resource/blend_tree_motion.hpp"

class DocBlendTree : public EditorDocumentTyped<BlendTree> {
    GuiViewport viewport;
    GameScene scn;

    ktNode* cam_pivot = 0;
    DirLight* cam_light = 0;

    BlendTreeMotion motion;

    JobGraphNode* selected_node = 0;

    void setReferenceObject(ktNode* node);

    void guiDrawNode(JobGraph& jobGraph, JobGraphNode* node, ImVec2* pos);

public:
    DocBlendTree();
    DocBlendTree(std::shared_ptr<ResourceNode>& node);
    
    void onResourceSet() override;
    void onPreSave() override;

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;
};

#endif
