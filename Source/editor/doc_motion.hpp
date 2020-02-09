#ifndef DOC_MOTION_HPP
#define DOC_MOTION_HPP

#include "editor_document.hpp"

#include "../common/resource/motion.hpp"

class DocMotion : public EditorDocumentTyped<Motion> {
public:
    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;
};
STATIC_RUN(DocMotion) {
    regEditorDocument<DocMotion>({ "motion" });
}

#endif
