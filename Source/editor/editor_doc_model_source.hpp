#ifndef EDITOR_DOC_MODEL_SOURCE_HPP
#define EDITOR_DOC_MODEL_SOURCE_HPP

#include "editor_document.hpp"

#include "../common/gui_viewport.hpp"

#include "../common/resource/model_source.hpp"

class EditorDocModelSource : public EditorDocumentTyped<ModelSource> {
    bool first_use = true;
    GuiViewport gvp;
public:
    EditorDocModelSource(std::shared_ptr<ResourceNode>& node);

    virtual void onGui(Editor* ed, float dt);
    void onGuiToolbox(Editor* ed) override;
};

#endif
