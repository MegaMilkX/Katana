#ifndef DOC_BLEND_TREE_HPP
#define DOC_BLEND_TREE_HPP

#include "editor_document.hpp"
#include "../common/resource/blend_tree.hpp"
#include "../common/resource/resource_tree.hpp"

class DocBlendTree : public EditorDocumentTyped<BlendTree> {
public:

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;
};

#endif
