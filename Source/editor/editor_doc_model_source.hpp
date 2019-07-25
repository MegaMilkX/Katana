#ifndef EDITOR_DOC_MODEL_SOURCE_HPP
#define EDITOR_DOC_MODEL_SOURCE_HPP

#include "editor_document.hpp"

#include "../common/gui_viewport.hpp"

#include "../common/resource/model_source.hpp"

class EditorDocModelSource : public EditorDocument {
    bool first_use = true;
    GuiViewport gvp;
    std::shared_ptr<ModelSource> mdl_src;
public:
    EditorDocModelSource(ResourceNode* node);

    virtual void onGui(Editor* ed);
};

#endif
