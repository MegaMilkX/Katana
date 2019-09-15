#ifndef DOC_MATERIAL_HPP
#define DOC_MATERIAL_HPP

#include "editor_document.hpp"
#include "../common/resource/material.hpp"
#include "../common/resource/resource_tree.hpp"

class DocMaterial : public EditorDocumentTyped<Material> {
public:
    DocMaterial();

    void onGui(Editor* ed, float dt);
};
STATIC_RUN(DocMaterial) {
    regEditorDocument<DocMaterial>("mat");
}

#endif
