#ifndef DOC_BLEND_TREE_HPP
#define DOC_BLEND_TREE_HPP

#include "editor_document.hpp"
#include "../common/resource/blend_tree.hpp"
#include "../common/resource/resource_tree.hpp"

#include "../common//scene/game_scene.hpp"
#include "../common/gui_viewport.hpp"

#include "../common/resource/skeleton.hpp"
#include "../common/resource/animation.hpp"

class DocBlendTree : public EditorDocumentTyped<BlendTree> {
    GuiViewport viewport;
    GameScene scn;
    std::shared_ptr<GameScene> ref_scn;
    std::shared_ptr<Skeleton> skel;
    std::vector<std::shared_ptr<Animation>> clips;
    ktNode* cam_pivot = 0;
public:
    DocBlendTree();

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;
};

#endif
