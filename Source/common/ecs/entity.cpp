#include "entity.hpp"

#include "world.hpp"

void ecsEntity::signalAttribUpdate(ecsWorld* world, uint8_t attrib_id) {
    uint64_t attrib_mask = (1 << attrib_id);
    auto tuple_map_ref = first_tuple_map.get();
    while(tuple_map_ref != 0) {
        uint64_t tuple_mask = tuple_map_ref->ptr->get_mask() | tuple_map_ref->ptr->get_opt_mask();
        if(tuple_mask & attrib_mask) {
            if(tuple_map_ref->tuple->array_index < tuple_map_ref->ptr->dirty_index) {
                tuple_map_ref->ptr->markDirty(tuple_map_ref->tuple->array_index);
            }
            tuple_map_ref->tuple->signalAttribUpdate(attrib_mask);
        }
        tuple_map_ref = tuple_map_ref->next.get();
    }
}