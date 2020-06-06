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

class ecsWorld;
class ecsEntity {
    friend ecsWorld;

    entity_id parent_uid = NULL_ENTITY;
    entity_id first_child_uid = NULL_ENTITY;
    entity_id next_sibling_uid = NULL_ENTITY;

    entity_id entity_uid;
    uint64_t attrib_bits;
    std::map<uint8_t, std::shared_ptr<ecsAttribBase>> attribs;
    uint64_t bitmaskInheritedAttribs = 0;

    std::unique_ptr<ecsTupleMapReference> first_tuple_map;

    ecsAttribBase* allocAttribOwned(ecsWorld* world, attrib_id id) {
        auto ptr = allocAttrib(id);
        ptr->h_entity = ecsEntityHandle(world, entity_uid);
        return ptr;
    }

    ecsTupleBase* findTupleOfSameContainer(ecsTupleMapBase* container) {
        auto map = first_tuple_map.get();
        while(map != 0) {
            if(map->ptr == container) {
                return map->tuple;
            }
            map = map->next.get();
        }
    }

public:
    template<typename T>
    T* getAttrib() {
        return dynamic_cast<T*>(getAttrib(T::get_id_static()));
    }

    ecsAttribBase* getAttrib(ecsWorld* world, attrib_id id) {
        ecsAttribBase* ptr = findAttrib(id); // TODO: ???
        ptr = allocAttribOwned(world, id);
        attribs[(uint8_t)id].reset(ptr);
        setBit(id);
        return ptr;
    }
    template<typename T>
    T* setAttrib(ecsWorld* world, const T& value) {
        T* ptr = findAttrib<T>();
        if(!ptr) {
            ptr = (T*)allocAttribOwned(world, T::get_id_static());
            if(!ptr) {
                return 0;
            }
            attribs[(uint8_t)T::get_id_static()].reset(ptr);
            setBit(T::get_id_static());
        }
        *ptr = value;
        return ptr;
    }

    void removeAttrib(attrib_id id) {
        attribs.erase((uint8_t)id);
        clearBit(id);
    }

    template<typename T>
    T* findAttrib() {
        return dynamic_cast<T*>(findAttrib(T::get_id_static()));
    }

    ecsAttribBase* findAttrib(attrib_id id) {
        uint64_t mask = 1 << id;
        if(attrib_bits & mask) {
            return attribs[id].get();
        }
        return 0;
    }

    ecsAttribBase* getAttribPtr(uint8_t attrib_id) {
        auto it = attribs.find(attrib_id);
        if(it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }

    template<typename T>
    bool updateAttrib(const T& value) {
        auto it = attribs.find(T::get_id_static());
        if(it == attribs.end()) {
            return false;
        }
        *(T*)(it->second.get()) = (value);        
        return true;
    }

    void signalAttribUpdate(ecsWorld* world, uint8_t attrib_id);

    const uint64_t& getAttribBits() const {
        return attrib_bits;
    }
    void setBit(attrib_id attrib) {
        attrib_bits |= (1 << attrib);
    }
    void clearBit(attrib_id attrib) {
        attrib_bits &= ~(1 << attrib);
    }

};

void recursiveTupleMarkDirty(ecsTupleMapBase* container, ecsTupleBase* tuple, uint64_t attrib_id);

#endif
