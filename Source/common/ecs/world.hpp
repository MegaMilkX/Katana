#ifndef ECS_WORLD__HPP
#define ECS_WORLD__HPP

#include "../resource/resource.h"

#include "system.hpp"

#include "../util/object_pool.hpp"

#include <memory>

#include "../util/timer.hpp"

#include "entity_handle.hpp"

#include "storage/world_storage.hpp"

#include "actor.hpp"
/*
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
*/
inline size_t calcArchetypeSize(uint64_t attribmask) {
    size_t sz = 0;
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        if((attribmask & (1ULL << i)) == 0) continue;
        auto info = getEcsAttribTypeLib().get_info(i);
        if(!info) break;
        sz += info->size_of;
    }
    return sz;
}

inline size_t archetypeSize(uint64_t attrib_mask) {
    size_t sz = 0;
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        sz += getEcsAttribTypeLib().get_info(i)->size_of * ((attrib_mask & (1ULL << i)) >> i);
    }
    return sz;
}

inline int64_t archetypeOffset(uint64_t attrib_mask, attrib_id attr) {
    int64_t offset = 0;
    if (attrib_mask & (1ULL << attr) == 0) {
        return  -1;
    }
    for(size_t i = 0; i < attr; ++i) {
        if ((attrib_mask & (1ULL << i)) == 0) {
            continue;
        }
        auto inf = getEcsAttribTypeLib().get_info(i);
        offset += inf->size_of;
    }
    return offset;
}

#include "types.hpp"
#include "../resource/entity_template.hpp"
#include "entity_storage.hpp"

struct ArchetypeStorage {
    ArchetypeStorage(uint64_t signature) // for dynamic entities
    : signature(signature), storage(archetypeSize(signature)) {
        std::fill(attrib_offsets, attrib_offsets + sizeof(attrib_offsets) / sizeof(attrib_offsets[0]), -1);
        entity_size = archetypeSize(signature);
        for(int i = 0; i < 64; ++i) {
            uint64_t bit = (1ULL << i);
            if ((signature & bit) == 0) {
                continue;
            }
            auto& offs = attrib_offsets[i];
            offs = archetypeOffset(signature, i);
        }
    }
    ArchetypeStorage(uint64_t signature, int64_t* offset_table, uint64_t entity_size) // for static entities (actors)
    : signature(signature), storage(entity_size) { 
        this->entity_size = entity_size;
        memcpy(attrib_offsets, offset_table, sizeof(attrib_offsets));
    }
    uint64_t entity_size;
    uint64_t signature;
    int64_t attrib_offsets[64];
    EntityStorage storage;
    std::vector<entity_id> storage_to_entity;
};

// Call constructors on all attributes
inline void constructEntityData(void* data, uint64_t sig, ecsWorld* world, entity_id e) {
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        if(sig & (1ULL << i)) {
            auto* inf = getEcsAttribTypeLib().get_info(i);
            auto offset = archetypeOffset(sig, i);
            auto attrib_ptr = (ecsAttribBase*)((uint8_t*)data + offset);
            inf->constructor_in_place(attrib_ptr);
            attrib_ptr->h_entity = ecsEntityHandle(world, e);
        }
    }
}
// Copy construct mutual attributes, construct attributes that are present in dest, but not src
inline void copyConstructEntityData(void* dest, uint64_t dest_sig, void* src, uint64_t src_sig, ecsWorld* world, entity_id e) {
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        if(dest_sig & src_sig & (1ULL << i)) {
            auto* inf = getEcsAttribTypeLib().get_info(i);
            auto dest_offset = archetypeOffset(dest_sig, i);
            auto src_offset  = archetypeOffset(src_sig, i);
            inf->copy_constructor((ecsAttribBase*)((uint8_t*)dest + dest_offset), (ecsAttribBase*)((uint8_t*)src + src_offset));
        } else if (dest_sig & (1ULL << i)) {
            auto* inf = getEcsAttribTypeLib().get_info(i);
            auto dest_offset = archetypeOffset(dest_sig, i);
            auto attrib_ptr = (ecsAttribBase*)((uint8_t*)dest + dest_offset);
            inf->constructor_in_place(attrib_ptr);
            attrib_ptr->h_entity = ecsEntityHandle(world, e);
        }
    }
}
// Call destructors for all attributes
inline void deleteEntityData(void* data, uint64_t sig) {
    for(size_t i = 0; i < get_last_attrib_id() + 1; ++i) {
        if(sig & (1ULL << i)) {
            auto* inf = getEcsAttribTypeLib().get_info(i);
            auto offset = archetypeOffset(sig, i);
            auto ptr = (ecsAttribBase*)((uint8_t*)data + offset);
            ptr->~ecsAttribBase();
        }
    }
}

ecsWorld* derefWorldIndex(int32_t idx);

#include "actors/actor.hpp"

struct ecsWorldArchetypes {
    std::unique_ptr<ArchetypeStorage> dynamic_storage; // dynamic entity storage
    std::unordered_map<rttr::type, std::unique_ptr<ArchetypeStorage>> actor_storages; // static actor storage (can't add or remove attributes)
};

class ecsWorld : public Resource {
    int32_t                                     pool_index = -1;

    ecsEntityHandle                             parent_world_entity;
    ecsWorld*                                   parent_world            = 0;

    uint64_t                                    global_attrib_mask      = 0;
    std::vector<int>                            global_attrib_counters;

    ObjectPool<ecsEntity>                       entities;
    std::set<entity_id>                         live_entities;

    std::unordered_map<uint64_t, ecsWorldArchetypes> archetype_storages; // key - signature
    ArchetypeStorage* getArchStorage(uint64_t signature);
    
    std::vector<std::unique_ptr<ecsSystemBase>> systems;
    std::map<rttr::type, size_t>                sys_by_type;

    void onAttribsCreated(entity_id ent, uint64_t entity_sig, uint64_t diff_sig);
    void onAttribsRemoved(entity_id ent, uint64_t entity_sig, uint64_t diff_sig);

    std::unordered_map<entity_id, std::shared_ptr<EntityTemplate>> entity_to_template;
    std::unordered_map<std::shared_ptr<EntityTemplate>, std::set<entity_id>> template_to_entities;
    void setTemplateLink(entity_id e, std::shared_ptr<EntityTemplate> tpl);
    void clearTemplateLink(entity_id e);
    void tryClearTemplateLink(entity_id e);

    std::unordered_map<rttr::type, std::unique_ptr<ecsWorldStorage>> storages;

    template<typename T>
    T* addSystem() {
        sys_by_type[rttr::type::get<T>()] = systems.size();
        T* sys = new T();
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        ((ecsSystemBase*)sys)->world = this;
        for(auto e : live_entities) {
            ecsEntity* ent = entities.deref(e);
            if (ent->storage) {
                sys->attribsCreated(this, e, ent->storage->signature, ent->storage->signature);
            }
        }
        return sys;
    }
    template<typename T, typename... Args>
    T* addSystem(Args... arg) {
        sys_by_type[rttr::type::get<T>()] = systems.size();
        T* sys = new T(std::forward<Args>(arg)...);
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        ((ecsSystemBase*)sys)->world = this;
        for(auto e : live_entities) {
            ecsEntity* ent = entities.deref(e);
            if (ent->storage) {
                sys->attribsCreated(this, e, ent->storage->signature, ent->storage->signature);
            }
        }
        return sys;
    }

public:
    void                        _linkTupleContainer         (entity_id e, ecsTupleMapBase* tuple_map, ecsTupleBase* tuple);
    void                        _unlinkTupleContainer       (entity_id e, ecsTupleMapBase* tuple_map);
    void                        _findTreeRelations          (entity_id e, ecsTupleMapBase* tuple_map, ecsTupleBase* tuple);

    ecsWorld();
    ~ecsWorld();

    int32_t                     getWorldIndex               (void) const { return pool_index; }

    void                        clearEntities               (void);
    void                        clearSystems                (void);

    template<typename T>
    T*                          createActor                 ();

    ecsEntityHandle             createEntity                ();
    ecsEntityHandle             createEntity                (archetype_mask_t attrib_signature);
    ecsEntityHandle             createEntityFromTemplate    (const char* tplPath);
    void                        removeEntity                (entity_id id);
    void                        removeTree                  (entity_id id);
    const std::set<entity_id>&  getEntities() const;
    ecsEntityHandle             findEntity                  (const char* name);
    ecsEntityHandle             findChild                   (entity_id parent, const char* name);

    void                        setParent                   (entity_id parent, entity_id child);
    entity_id                   getParent                   (entity_id e);
    entity_id                   getFirstChild               (entity_id e);
    entity_id                   getNextSibling              (entity_id e);
    int                         getTreeDepth                (entity_id e);

    ecsEntityHandle             mergeWorld                  (const char* res_name);
    ecsEntityHandle             mergeWorld                  (ecsWorld* world);

    void                        setParentWorldEntity        (ecsEntityHandle hdl) { parent_world_entity = hdl; }
    ecsEntityHandle             getParentWorldEntity        (void) const { return parent_world_entity; }

    template<typename T>
    T*                          findAttrib(entity_id ent);
    template<typename T>
    T*                          getAttrib(entity_id ent);
    template<typename T>
    T*                          setAttrib(entity_id ent, const T& value);
    template<typename T>
    void                        createAttrib(entity_id ent);
    void                        createAttrib(entity_id ent, attrib_id attrib);
    void                        setAttribInheritanceMask(entity_id ent, uint64_t mask);
    void                        clearAttribInheritance(entity_id e, attrib_id attrib);
    void                        clearAllAttribInheritance(entity_id e);
    template<typename T>
    void                        removeAttrib(entity_id ent);
    void                        removeAttrib(entity_id ent, attrib_id attrib);
    void                        removeAttribs(entity_id ent, uint64_t mask);

    ecsAttribBase*              findAttrib(entity_id ent, attrib_id attrib);

    ecsAttribBase*              getAttribPtr(entity_id ent, attrib_id id);

    void                        copyAttribs                 (entity_id dst, ecsEntityHandle src, uint64_t ignore_mask = 0);

    uint64_t                    getAttribBitmask(entity_id e);
    uint64_t                    getInheritedAttribBitmask(entity_id e);

    template<typename T>
    void                        signalAttribUpdate(entity_id ent);

    void                        signalAttribUpdate(entity_id ent, attrib_id attrib);

    template<typename T>
    void                        updateAttrib(entity_id ent, const T& value);

    EntityTemplate*             updateTemplate(entity_id ent);
    void                        updateDerived(std::shared_ptr<EntityTemplate> tpl);
    void                        linkToTemplate(entity_id ent, std::shared_ptr<EntityTemplate> tpl);
    void                        resetToTemplate(entity_id ent);

    template<typename T>
    T*                          getStorage();

    template<typename T>
    T*                          getSystem();
    template<typename T, typename... Args>
    T*                          getSystem(Args... arg);
    int                         systemCount();
    ecsSystemBase*              getSystem(int i);

    template<typename ENTITY_VIEW_T>
    void                        forEachEntity(std::function<void(ENTITY_VIEW_T& view)> fn);

    void                        update();


    void                        serialize(out_stream& out) override;
    bool                        deserialize(in_stream& in, size_t sz) override;
    bool                        deserialize(in_stream& in, size_t sz, entity_id merge_parent);

    static void                 serializeAttribDesc     (ecsWorldWriteCtx& ctx);
    static void                 deserializeAttribDesc   (ecsWorldReadCtx& ctx);
    static void                 serializeEntity         (ecsWorldWriteCtx& ctx, entity_id e, bool keep_template_link = false);
    static void                 deserializeEntity       (ecsWorldReadCtx& ctx, entity_id e, uint64_t attrib_ignore_mask = 0);

    const char*                 getWriteExtension() const override { return "ecsw"; }

};
STATIC_RUN(ecsWorld) {
    rttr::registration::class_<ecsWorld>("ecsWorld")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

template<typename T>
T* ecsWorld::createActor() {
    auto desc = actorGetDesc<T>();
    auto it = archetype_storages.find(desc->signature);
    if(it == archetype_storages.end()) {
        archetype_storages.insert(std::make_pair(desc->signature, ecsWorldArchetypes() ));
        it = archetype_storages.find(desc->signature);
        it->second.dynamic_storage.reset(new ArchetypeStorage(desc->signature));
    }
    auto& actor_map = it->second.actor_storages;
    auto it_actor_storage = actor_map.find(rttr::type::get<T>());
    if(it_actor_storage == actor_map.end()) {
        actor_map.insert(std::make_pair(rttr::type::get<T>(), std::unique_ptr<ArchetypeStorage>(new ArchetypeStorage(desc->signature, desc->offset_table, sizeof(T)))));
        it_actor_storage = actor_map.find(rttr::type::get<T>());
    }
    auto storage_ptr = it_actor_storage->second.get();
    auto actor_entity_data_idx = storage_ptr->storage.alloc();
    void* actor_ptr = storage_ptr->storage.deref(actor_entity_data_idx);
    new (actor_ptr) T();

    entity_id e = entities.acquire();
    ecsEntity* entity = entities.deref(e);
    entity->storage = storage_ptr;
    entity->bitmaskInheritedAttribs = 0;
    entity->first_child_uid = ENTITY_ERROR;
    entity->next_sibling_uid = ENTITY_ERROR;
    entity->parent_uid = ENTITY_ERROR;
    entity->storage_index = actor_entity_data_idx;

    // Set attributes's handles to their entity
    // TODO: Need to get rid of this handle, it's a waste of memory
    for(int i = 0; i < 64; ++i) {
        auto offset = desc->offset_table[i];
        uint8_t* actor_base_ptr = (uint8_t*)actor_ptr;
        if(offset == -1) {
            continue;
        }
        ecsAttribBase* attrib = (ecsAttribBase*)(actor_base_ptr + offset);
        attrib->h_entity = ecsEntityHandle(this, e);
    }

    live_entities.insert(e);

    onAttribsCreated(e, desc->signature, desc->signature);
}

template<typename T>
T* ecsWorld::findAttrib(entity_id ent) {
    return (T*)findAttrib(ent, T::get_id_static());
}
template<typename T>
T* ecsWorld::getAttrib(entity_id ent) {
    auto e = entities.deref(ent);
    auto a = findAttrib<T>(ent);
    if(!a) {
        createAttrib<T>(ent);
        a = findAttrib<T>(ent);
    }
    return a;
}
template<typename T>
T* ecsWorld::setAttrib(entity_id ent, const T& value) {
    auto e = entities.deref(ent);
    auto a = findAttrib<T>(ent);
    if(!a) {
        createAttrib<T>(ent);
        updateAttrib(ent, value);
        a = findAttrib<T>(ent);
        
        global_attrib_counters[T::get_id_static()]++;
        global_attrib_mask |= (1ULL << T::get_id_static());
        
        onAttribsCreated(ent, e->storage->signature, 1ULL << T::get_id_static());
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
    auto attrib_id = T::get_id_static();
    if (!e->storage) {
        assert(false);
        return;
    }
    auto attrib_ptr = findAttrib(ent, attrib_id);
    assert(attrib_ptr);
    T::attrib_info->copy_constructor(attrib_ptr, &value);

    e->signalAttribUpdate(this, T::get_id_static());
}

template<typename T>
T* ecsWorld::getStorage() {
    auto it = storages.find(rttr::type::get<T>());
    if(it == storages.end()) {
        T* storage = new T;
        storages[rttr::type::get<T>()] = std::unique_ptr<T>(storage);
        return storage;
    }
    return (T*)it->second.get();
}

template<typename T>
T* ecsWorld::getSystem() {
    auto it = sys_by_type.find(rttr::type::get<T>());
    if(it != sys_by_type.end()) {
        return (T*)systems[it->second].get();
    }
    return addSystem<T>();
}
template<typename T, typename... Args>
T* ecsWorld::getSystem(Args... arg) {
    auto it = sys_by_type.find(rttr::type::get<T>());
    if(it != sys_by_type.end()) {
        return (T*)systems[it->second].get();
    }
    return addSystem<T, Args...>(std::forward<Args>(arg)...);
}


template<typename ENTITY_VIEW_T>
void ecsWorld::forEachEntity(std::function<void(ENTITY_VIEW_T& view)> fn) {
    // TODO: pointers to storages should be stored in persistent filters?
    std::vector<ArchetypeStorage*> storages;
    for(auto& it : archetype_storages) {
        if((it.first & ENTITY_VIEW_T::get_signature()) == ENTITY_VIEW_T::get_signature()) {
            storages.push_back(it.second.dynamic_storage.get());
            for(auto& it2 : it.second.actor_storages) {
                storages.push_back(it2.second.get());
            }
        }
    }

    ENTITY_VIEW_T view;
    for(ArchetypeStorage* storage : storages) {
        if(storage->storage.count() == 0) {
            continue;
        }
        for(int i = 0; i < storage->storage.chunkCount(); ++i) {                // for each chunk
            auto elemCount = storage->storage.chunkElemCount(i);
            for(int j = 0; j < elemCount; ++j) {                                // for each element in that chunk
                void* base_ptr = storage->storage.derefChunkElement(i, j);
                for(int k = 0; k < view.attrib_pointers.size(); ++k) {          // set attrib pointers
                    auto attrib_idx = view.attrib_indices[k];
                    auto attrib_offs = storage->attrib_offsets[attrib_idx];

                    if(attrib_offs != -1) {
                        *view.attrib_pointers[k] = (ecsAttribBase*)((uint8_t*)base_ptr + attrib_offs);
                    } else {
                        *view.attrib_pointers[k] = 0;
                    }
                }
                fn(view);                                                       // call function
            }
        }
    }
}


#endif
