#ifndef ECS_WORLD__HPP
#define ECS_WORLD__HPP

#include "../resource/resource.h"

#include "system.hpp"

#include "../util/object_pool.hpp"

#include <memory>

#include "../util/timer.hpp"

#include "entity_handle.hpp"

class byte_block_vector {
    char** _blocks = 0;
    size_t _elem_size = 0;
    size_t _elem_per_block = 0;
    size_t _block_count = 0;
    size_t _count = 0;
    
    void deconstruct_id(size_t id, size_t& out_block_id, size_t& out_local_item_id) const {
        out_block_id = id / _elem_per_block;
        out_local_item_id = id - _elem_per_block * out_block_id;
    }
public:
    byte_block_vector(size_t elem_size, size_t elems_per_block)
    : _elem_size(elem_size), _elem_per_block(elems_per_block) {
        _blocks = new char*[1];
        void* blockptr = malloc(_elem_size * _elem_per_block);
        _blocks[0] = (char*)blockptr;
        _block_count = 1;
    }
    
    size_t elem_size() const { return _elem_size; }
    size_t block_count() const { return _block_count; }
    size_t size() const { return _count; }
    
    void clear() {
        for(size_t i = 0; i < _block_count; ++i) {
            delete _blocks[i];
        }
        delete[] _blocks;
        _blocks = 0;
    }

    void expand(size_t count) {
        if(count < _count) {
            return;
        }

        size_t blockid = 0;
        size_t localid = 0;
        deconstruct_id(count - 1, blockid, localid);
        size_t new_block_count = blockid + 1;
        char** new_block_ptrs = new char*[new_block_count];
        for(size_t i = 0; i < _block_count; ++i) {
            new_block_ptrs[i] = _blocks[i];
        }
        for(size_t i = _block_count; i < new_block_count; ++i) {
            new_block_ptrs[i] = (char*)malloc(_elem_size * _elem_per_block);
        }
        delete[]_blocks;

        _count = count;
        _blocks = new_block_ptrs;
        _block_count = new_block_count;
    }

    const void* operator[](size_t index) const {
        size_t blockid = 0;
        size_t localid = 0;
        deconstruct_id(index, blockid, localid);
        return _blocks[blockid] + localid * _elem_size;
    }
    void*       operator[](size_t index) {
        size_t blockid = 0;
        size_t localid = 0;
        deconstruct_id(index, blockid, localid);
        return _blocks[blockid] + localid * _elem_size;
    }
};

inline size_t calcArchetypeSize(uint64_t attribmask) {
    size_t sz = 0;
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        if(attribmask & (1 << i) == 0) continue;
        auto info = getEcsAttribTypeLib().get_info(i);
        if(!info) break;
        sz += info->size_of;
    }
    return sz;
}

inline size_t archetypeSize(uint64_t attrib_mask) {
    size_t sz = 0;
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        sz += getEcsAttribTypeLib().get_info(i)->size_of * ((attrib_mask & (1 << i)) >> i);
    }
    return sz;
}

inline int64_t archetypeOffset(uint64_t attrib_mask, uint64_t attrib_id) {
    int64_t offset = -1;
    for(size_t i = 0; i < attrib_id; ++i) {
        offset += getEcsAttribTypeLib().get_info(i)->size_of * ((attrib_mask & (1 << i)) >> i);
    }
    return offset;
}


typedef uint64_t archetype_mask_t;

class ecsWorld : public Resource {
    ObjectPool<ecsEntity>                       entities;
    std::vector<std::unique_ptr<ecsSystemBase>> systems;
    std::map<rttr::type, size_t>                sys_by_type;

    std::set<entity_id>                         live_entities;

    template<typename T>
    T* addSystem() {
        sys_by_type[rttr::type::get<T>()] = systems.size();
        T* sys = new T();
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        ((ecsSystemBase*)sys)->world = this;
        for(auto e : live_entities) {
            ecsEntity* ent = entities.deref(e);
            sys->attribsCreated(this, e, ent->getAttribBits(), ent->getAttribBits());
        }
        return sys;
    }

public:
    ecsEntityHandle     createEntity();
    ecsEntityHandle     createEntity(archetype_mask_t attrib_signature);
    void                removeEntity(entity_id id);

    const std::set<entity_id>&  getEntities() const;

    template<typename T>
    T* findAttrib(entity_id ent);
    template<typename T>
    T* getAttrib(entity_id ent);
    template<typename T>
    T* setAttrib(entity_id ent, const T& value);
    template<typename T>
    void createAttrib(entity_id ent);
    void createAttrib(entity_id ent, attrib_id attrib);
    template<typename T>
    void removeAttrib(entity_id ent);
    void removeAttrib(entity_id ent, attrib_id attrib);

    ecsAttribBase* getAttribPtr(entity_id ent, attrib_id id);

    template<typename T>
    void signalAttribUpdate(entity_id ent);

    void signalAttribUpdate(entity_id ent, attrib_id attrib);

    template<typename T>
    void updateAttrib(entity_id ent, const T& value);

    template<typename T>
    T* getSystem();

    void update();


    void serialize(out_stream& out) override;
    bool deserialize(in_stream& in, size_t sz) override;

    const char* getWriteExtension() const override { return "ecsw"; }

};
STATIC_RUN(ecsWorld) {
    rttr::registration::class_<ecsWorld>("ecsWorld")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

template<typename T>
T* ecsWorld::findAttrib(entity_id ent) {
    auto e = entities.deref(ent);
    auto a = e->findAttrib<T>();
    return a;
}
template<typename T>
T* ecsWorld::getAttrib(entity_id ent) {
    auto e = entities.deref(ent);
    auto a = e->findAttrib<T>();
    if(!a) {
        createAttrib<T>(ent);
        a = e->findAttrib<T>();
    }
    return a;
}
template<typename T>
T* ecsWorld::setAttrib(entity_id ent, const T& value) {
    auto e = entities.deref(ent);
    auto a = e->findAttrib<T>();
    if(!a) {
        e->setAttrib<T>(value);
        a = e->findAttrib<T>();
        for(auto& sys : systems) {
            sys->attribsCreated(this, ent, e->getAttribBits(), 1 << T::get_id_static());
        }
    } else {
        updateAttrib(ent, value);
    }
    return a;
}
template<typename T>
void ecsWorld::createAttrib(entity_id ent) {
    createAttrib(ent, T::get_id_static());
}
template<typename T>
void ecsWorld::removeAttrib(entity_id ent) {
    removeAttrib(ent, T::get_id_static());
}

template<typename T>
void ecsWorld::signalAttribUpdate(entity_id ent) {
    signalAttribUpdate(ent, T::get_id_static());
}

template<typename T>
void ecsWorld::updateAttrib(entity_id ent, const T& value) {
    auto e = entities.deref(ent);
    if(e->updateAttrib(value)) {
        for(auto& sys : systems) {
            sys->signalUpdate(ent, 1 << T::get_id_static());
        }
    }
}

template<typename T>
T* ecsWorld::getSystem() {
    auto it = sys_by_type.find(rttr::type::get<T>());
    if(it != sys_by_type.end()) {
        return (T*)systems[it->second].get();
    }
    return addSystem<T>();
}

#endif
