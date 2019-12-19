#ifndef DOC_BLEND_TREE_2_HPP
#define DOC_BLEND_TREE_2_HPP


#include "editor_document.hpp"
#include "../common/resource/blend_tree_2.hpp"


class DocBlendTree2 : public EditorDocumentTyped<BlendTree2> {


public:
    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;

};


#endif
