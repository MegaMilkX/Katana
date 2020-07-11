#ifndef DOC_ECS_WORLD_STATE_HPP
#define DOC_ECS_WORLD_STATE_HPP

#include "../../common/ecs/world.hpp"

#include "../../common/ecs/attribs/base_attribs.hpp"

#include "../../common/ecs/systems/dynamics.hpp"
#include "../../common/ecs/systems/render.hpp"
#include "../../common/ecs/systems/render_gui.hpp"

#include "../../common/renderer.hpp"
#include "../../common/gui_viewport.hpp"

#include "../editor_async_task/editor_async_task.hpp"


static const int UNDO_STACK_MAX = 35;

struct DocEcsWorldState {
    typedef std::vector<std::shared_ptr<ecsWorld>> undo_stack_t;

    DocEcsWorldState::undo_stack_t* undo_stack = 0;

    ecsWorld*           world = 0;
    entity_id           selected_ent = 0;
    GuiViewport         gvp;
    DrawList            dl;

    std::set<std::unique_ptr<edTaskEcsWorldModelDragNDrop>> model_dnd_tasks;

    void backupState() {
        assert(undo_stack);
        if(undo_stack->size() == UNDO_STACK_MAX) {
            undo_stack->erase(undo_stack->begin());
        }

        undo_stack->push_back(std::shared_ptr<ecsWorld>(new ecsWorld()));
        dstream strm;
        world->serialize(strm);
        strm.jump(0);
        undo_stack->back()->deserialize(strm, strm.bytes_available());
    }

    void restoreState() {
        assert(undo_stack);
        if(undo_stack->empty()) {
            return;
        }

        dstream strm;
        undo_stack->back()->serialize(strm);
        undo_stack->pop_back();
        strm.jump(0);
        world->clearEntities();
        world->deserialize(strm, strm.bytes_available());

        selected_ent = 0; // TODO: Grab selected entity from world's editor payload (when it's done)
    }
};


#endif
