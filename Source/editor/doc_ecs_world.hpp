#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/ecs/world.hpp"

#include "../common/util/mesh_gen.hpp"

#include "doc_ecs_world/mode.hpp"
#include "doc_ecs_world/mode_3d.hpp"
#include "doc_ecs_world/mode_gui.hpp"


#include "../common/util/bullet_debug_draw.hpp"


#include "../common/util/block_vector.hpp"


#include "../common/ecs/systems/scene_graph.hpp"
#include "../common/ecs/systems/animation_sys.hpp"
#include "../common/ecs/attribs/sub_scene_animator.hpp"

#include "../common/input_listener.hpp"

#include "../common/render_test.hpp"

#include "../common/kt_cmd.hpp"

#include "../common/util/filesystem.hpp"


class DocEcsWorld : public EditorDocumentTyped<ecsWorld>, public InputListenerWrap {
    struct SubWorldContext {
        ecsWorld* world;
        DocEcsWorldState::undo_stack_t undo_stack;
    };
    std::vector<std::shared_ptr<SubWorldContext>> subscene_stack;
    DocEcsWorldState::undo_stack_t root_world_undo_stack;

    std::shared_ptr<DocEcsWorldMode> mode;
    DocEcsWorldState                 state;

    ecsRenderSystem* renderSys;
    ecsysSceneGraph* sceneGraphSys;

    // key modifiers
    bool inp_mod_ctrl = false;

public:
    DocEcsWorld();

    void onResourceSet() override;

    void onGui(Editor* ed, float dt) override;
    void onGuiToolbox(Editor* ed) override;

    void onFocus() override;
    void onUnfocus() override;

    void onCmdSubdoc(int argc, const char* argv[]);

};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
