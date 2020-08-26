#ifndef ECS_ENTITY_HPP
#define ECS_ENTITY_HPP

#include <stdint.h>
#include <map>
#include <memory>

#include "attribute.hpp"
#include "entity_handle.hpp"
#include "tuple_map_base.hpp"
#include "tuple_base.hpp"


inline ecsAttribBase* allocAttrib(attrib_id id) {
    ecsAttribBase* ptr = 0;
    auto inf = getEcsAttribTypeLib().get_info(id);
    if(!inf) {
        LOG_WARN("Attribute info for id " << id << " doesn't exist");
        assert(false);
        return 0;
    }
    ptr = inf->constructor();
    if(!ptr) {
        LOG_WARN("Constructor for attrib " << inf->name << " failed");
        assert(false);
        return 0;
    }
    return ptr;
}

struct ecsTupleMapReference {
    ecsTupleBase*                           tuple = 0;
    ecsTupleMapBase*                        ptr = 0;
    std::unique_ptr<ecsTupleMapReference>   next;
};

class ArchetypeStorage;
class ecsWorld;
class ecsEntity {
    friend ecsWorld;

    ArchetypeStorage* storage = 0;
    int storage_index = 0;

    entity_id parent_uid = NULL_ENTITY;
    entity_id first_child_uid = NULL_ENTITY;
    entity_id next_sibling_uid = NULL_ENTITY;
    int       tree_depth = 0;

    uint64_t bitmaskInheritedAttribs = 0;

    std::unique_ptr<ecsTupleMapReference> first_tuple_map;

    ecsTupleBase* findTupleOfSameContainer(ecsTupleMapBase* container) {
        auto map = first_tuple_map.get();
        while(map != 0) {
            if(map->ptr == container) {
                return map->tuple;
            }
            map = map->next.get();
        }
        return 0;
    }

public:
    template<typename T>
    T* getAttrib() {
        return dynamic_cast<T*>(getAttrib(T::get_id_static()));
    }

    void signalAttribUpdate(ecsWorld* world, uint8_t attrib_id);
};

void recursiveTupleMarkDirty(ecsTupleMapBase* container, ecsTupleBase* tuple, uint64_t attrib_id);

#endif
