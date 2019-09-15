#ifndef EDITOR_DOC_TEXTURE2D_HPP
#define EDITOR_DOC_TEXTURE2D_HPP

#include "editor_document.hpp"
#include "../common/resource/texture2d.h"
#include "../common/resource/resource_tree.hpp"

class EditorDocTexture2d : public EditorDocumentTyped<Texture2D> {
public:
    virtual void onGui(Editor* ed, float dt);
};
STATIC_RUN(EditorDocTexture2d) {
    regEditorDocument<EditorDocTexture2d>({"png", "jpg", "jpeg", "jfif", "tga"});
}

#endif
