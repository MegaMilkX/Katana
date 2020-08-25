#include "entity.hpp"

#include "world.hpp"

void recursiveTupleMarkDirty(ecsTupleMapBase* container, ecsTupleBase* tuple, uint64_t attrib_mask) {
    if(tuple->array_index < container->dirty_index) {
        auto tuple_child = tuple->first_child;
        while(tuple_child) {
            recursiveTupleMarkDirty(container, tuple_child, attrib_mask);
            tuple_child = tuple_child->next_sibling;
        }

        container->markDirty(tuple->array_index);
    }
    tuple->dirty_signature |= attrib_mask;
}

void ecsEntity::signalAttribUpdate(ecsWorld* world, uint8_t attrib_id) {
    uint64_t attrib_mask = (1ULL << attrib_id);
    auto tuple_map_ref = first_tuple_map.get();
    while(tuple_map_ref != 0) {
        uint64_t tuple_mask = tuple_map_ref->ptr->get_mask() | tuple_map_ref->ptr->get_opt_mask();
        if(tuple_mask & attrib_mask) {
            recursiveTupleMarkDirty(tuple_map_ref->ptr, tuple_map_ref->tuple, attrib_mask);
            tuple_map_ref->tuple->signalAttribUpdate(attrib_mask);
        }
        tuple_map_ref = tuple_map_ref->next.get();
    }
}