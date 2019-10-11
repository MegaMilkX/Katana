#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
public:
    void onGui(Editor* ed, float dt) override {
    }
    void onGuiToolbox(Editor* ed) override {
        
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
