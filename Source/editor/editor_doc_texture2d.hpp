#ifndef EDITOR_DOC_TEXTURE2D_HPP
#define EDITOR_DOC_TEXTURE2D_HPP

#include "editor_document.hpp"
#include "../common/resource/texture2d.h"
#include "../common/resource/resource_tree.hpp"

class EditorDocTexture2d : public EditorDocumentTyped<Texture2D> {
public:
    EditorDocTexture2d(std::shared_ptr<ResourceNode>& node);

    virtual void onGui(Editor* ed);
};

#endif
