#ifndef DOC_BLEND_TREE_HPP
#define DOC_BLEND_TREE_HPP

#include "editor_document.hpp"
#include "../common/resource/blend_tree.hpp"
#include "../common/resource/resource_tree.hpp"

#include "../common//scene/game_scene.hpp"
#include "../common/gui_viewport.hpp"

#include "../common/resource/skeleton.hpp"
#include "../common/resource/animation.hpp"

#include "../common/util/func_graph/func_graph.hpp"

struct BlendSeq {
public:
    struct Item {
        Animation* anim = 0;
        float weight = 1.0f;
        std::vector<int32_t> mapping;
    };
    std::vector<Item> seq;
};

class DocBlendTree : public EditorDocumentTyped<BlendTree> {
    GuiViewport viewport;
    GameScene scn;
    std::shared_ptr<GameScene> ref_scn;
    std::shared_ptr<Skeleton> skel;
    ktNode* cam_pivot = 0;

    FuncGraph funcGraph;

    std::vector<DataNode<BlendSeq>*> clip_nodes;
    std::vector<DataNode<float>*> weight_nodes;
    std::vector<std::shared_ptr<Animation>> clips;
    ResultNode<BlendSeq>* result_node;

public:
    DocBlendTree();

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;
};

#endif
