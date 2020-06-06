#ifndef ECS_SCENE_GRAPH_HPP
#define ECS_SCENE_GRAPH_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"
#include "../storage/storage_transform.hpp"


class ecsysSceneGraph;
class ecsTupleSubGraph : public ecsTuple<ecsSubScene> {
public:
    ecsysSceneGraph* sub_system = 0;

    void onAttribUpdate(ecsSubScene* ss) {
        if(ss->getWorld()) {
            sub_system = ss->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }
};

class ecsysSceneGraph : public ecsSystem<
    tupleTransform,
    ecsTupleSubGraph
> {
    void onFit(ecsTupleSubGraph* sub_graph) {
        if(sub_graph->get<ecsSubScene>()->getWorld()) {
            sub_graph->sub_system = sub_graph->get<ecsSubScene>()->getWorld()->getSystem<ecsysSceneGraph>();
        }
    }

    void onUpdate() {
        for(auto& a : get_array<ecsTupleSubGraph>()) {
            if(!a->sub_system) continue;
            a->sub_system->onUpdate();
        }

        for(int i = get_dirty_index<tupleTransform>(); i < count<tupleTransform>(); ++i) {
            auto tuple = get<tupleTransform>(i);
            ecsTranslation* translation     = tuple->get_optional<ecsTranslation>();
            ecsRotation* rotation           = tuple->get_optional<ecsRotation>();
            ecsScale* scale                 = tuple->get_optional<ecsScale>();
            ecsWorldTransform* world        = tuple->get<ecsWorldTransform>();

            gfxm::mat4 local = gfxm::mat4(1.0f);
            if(translation) {
                local = gfxm::translate(local, translation->getPosition());
            }
            if(rotation) {
                local = local * gfxm::to_mat4(rotation->getRotation());
            }
            if(scale) {
                local = gfxm::scale(local, scale->getScale());
            }
            world->setTransform(local);

            if(tuple->get_parent()) {
                world->setTransform(tuple->get_parent()->get<ecsWorldTransform>()->getTransform() * world->getTransform());
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<tupleTransform>();
    }

};


#endif
