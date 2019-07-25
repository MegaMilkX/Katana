#ifndef EDITOR_DOC_TEXTURE2D_HPP
#define EDITOR_DOC_TEXTURE2D_HPP

#include "editor_document.hpp"
#include "../common/resource/texture2d.h"
#include "../common/resource/resource_tree.hpp"

class EditorDocTexture2d : public EditorDocument {
    std::shared_ptr<Texture2D> texture;
public:
    EditorDocTexture2d(ResourceNode* node);

    virtual void onGui(Editor* ed);
};

#endif
