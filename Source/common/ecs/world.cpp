#include "world.hpp"

#include "../util/data_writer.hpp"
#include "../util/data_reader.hpp"

#include "../resource/entity_template.hpp"
#include "attribs/base_attribs.hpp"

#include <stack>


std::vector<ecsWorld*> world_pool;
std::set<int32_t>          free_world_indices;
int32_t acquireWorldIndex(ecsWorld* w) {
    int32_t idx = -1;
    if(free_world_indices.empty()) {
        idx = world_pool.size();
        world_pool.push_back(w);
    } else {
        idx = *free_world_indices.begin();
        free_world_indices.erase(free_world_indices.begin());
        world_pool[idx] = w;
    }
    return idx;
}
void freeWorldIndex(int32_t i) {
    free_world_indices.insert(i);
    world_pool[i] = 0;
}
ecsWorld* derefWorldIndex(int32_t i) {
    return world_pool[i];
}


void unlinkTupleTreeRelations(ecsTupleBase* tuple) {
    if (tuple->parent) {
        tuple->parent->_removeChild(tuple);
    }
  
    auto t = tuple->first_child;
    while(t != 0) {
        t->parent = 0;
        auto next = t->next_sibling;
        t->prev_sibling = 0;
        t->next_sibling = 0;
        t = next;
    }    

    tuple->parent = 0;
    tuple->first_child = 0;
    tuple->prev_sibling = 0;
    tuple->next_sibling = 0;
}


ArchetypeStorage* ecsWorld::getArchStorage(uint64_t signature) {
    if (signature == 0) {
        return 0;
    }
    auto it = archetype_storages.find(signature);
    if(it != archetype_storages.end()) {
        return it->second.dynamic_storage.get();
    }
    auto* ptr = new ArchetypeStorage(signature);
    archetype_storages.insert(std::make_pair(signature, ecsWorldArchetypes{ std::unique_ptr<ArchetypeStorage>(ptr) }));
    return ptr;
}

void ecsWorld::onAttribsCreated(entity_id ent, uint64_t entity_sig, uint64_t diff_sig) {
    // Check removal by excluders and optional additions
    auto e = entities.deref(ent);
    auto tuple_map = e->first_tuple_map.get();
    while(tuple_map != 0) {
        auto tuple_map_ptr = tuple_map->ptr;
        uint64_t mask      = tuple_map->ptr->get_mask();
        uint64_t opt_mask  = tuple_map->ptr->get_opt_mask();
        uint64_t excl_mask = tuple_map->ptr->get_exclusion_mask();
        const bool requirements_satisfied = (entity_sig & mask) == mask;
        const bool added_optional = (opt_mask & diff_sig) != 0;
        const bool added_excluder = (excl_mask & diff_sig) != 0;

        if(added_excluder) {
            tuple_map_ptr->onUnfitProxy(tuple_map->tuple->array_index);

            unlinkTupleTreeRelations(tuple_map->tuple);
            
            tuple_map_ptr->erase(tuple_map->tuple->array_index);
            tuple_map = tuple_map->next.get();
            _unlinkTupleContainer(ent, tuple_map_ptr);
            continue;
        } else if(/*requirements_satisfied && */added_optional) {
            tuple_map->tuple->dirty_signature |= (diff_sig & opt_mask); // Set dirty flags for added optionals
        }
        tuple_map = tuple_map->next.get();
    }

    // Check additions
    for(auto& sys : systems) {
        sys->attribsCreated(this, ent, entity_sig, diff_sig);
    }
}
void ecsWorld::onAttribsRemoved(entity_id ent, uint64_t entity_sig, uint64_t diff_sig) {
    // Check removals
    auto e = entities.deref(ent);
    auto tuple_map = e->first_tuple_map.get();
    while(tuple_map != 0) {
        auto tuple_map_ptr = tuple_map->ptr;
        uint64_t mask = tuple_map_ptr->get_mask();
        uint64_t opt_mask = tuple_map_ptr->get_opt_mask();
        uint64_t excl_mask = tuple_map_ptr->get_exclusion_mask();

        const bool requirements_satisfied = (entity_sig & mask) == mask;
        const bool has_excluders = (entity_sig & excl_mask) != 0;
        const bool fits = requirements_satisfied && !has_excluders;
        const bool removed_requirement = (mask & diff_sig) != 0;
        const bool removed_optional = (opt_mask & diff_sig) != 0;
        const bool removed_excluder = (excl_mask & diff_sig) != 0;

        if(removed_requirement) {
            tuple_map_ptr->onUnfitProxy(tuple_map->tuple->array_index);

            unlinkTupleTreeRelations(tuple_map->tuple);

            tuple_map_ptr->erase(tuple_map->tuple->array_index);
            tuple_map = tuple_map->next.get();
            _unlinkTupleContainer(ent, tuple_map_ptr);
            continue;
        } else if (fits && removed_optional) {
            tuple_map->tuple->clearOptionals(diff_sig);
            tuple_map->tuple->dirty_signature &= ~(diff_sig & opt_mask); // Clear dirty flags for removed optionals
        }
        tuple_map = tuple_map->next.get();
    }

    // Check additions
    for(auto& sys : systems) {
        sys->attribsRemoved(this, ent, entity_sig, diff_sig);
    }
}


void ecsWorld::setTemplateLink(entity_id e, std::shared_ptr<EntityTemplate> tpl) {
    clearTemplateLink(e);
    entity_to_template[e] = tpl;
    template_to_entities[tpl].insert(e);
    tpl->insertReferencingWorld(this);
}
void ecsWorld::clearTemplateLink(entity_id e) {
    auto it = entity_to_template.find(e);
    if(it != entity_to_template.end()) {
        if (template_to_entities[it->second].empty()) {
            it->second->eraseReferencingWorld(this);
        }

        template_to_entities[it->second].erase(e);
        entity_to_template.erase(e);
    }
}
void ecsWorld::tryClearTemplateLink(entity_id e) {
    if(entities.deref(e)->bitmaskInheritedAttribs == 0) {
        // Can clear all ties to the template
        clearTemplateLink(e);
    }
}


void ecsWorld::_linkTupleContainer (entity_id e, ecsTupleMapBase* tuple_map, ecsTupleBase* tuple) {
    auto ent = entities.deref(e);
    auto tuple_map_ref = ent->first_tuple_map.get();
    if (tuple_map_ref == 0) {
        ent->first_tuple_map.reset(new ecsTupleMapReference());
        ent->first_tuple_map->tuple = tuple;
        ent->first_tuple_map->ptr = tuple_map;
        return;
    }
    while(tuple_map_ref != 0) {
        if(tuple_map_ref->next == 0) {
            tuple_map_ref->next.reset(new ecsTupleMapReference());
            tuple_map_ref->next->tuple = tuple;
            tuple_map_ref->next->ptr = tuple_map;
            break;
        }
        tuple_map_ref = tuple_map_ref->next.get();
    }
}
void ecsWorld::_unlinkTupleContainer (entity_id e, ecsTupleMapBase* tuple_map) {
    auto ent = entities.deref(e);
    auto tuple_map_ref = ent->first_tuple_map.get();
    if(tuple_map_ref->ptr == tuple_map) {
        ent->first_tuple_map.swap(tuple_map_ref->next);
    }
    while(tuple_map_ref != 0) {
        auto next = tuple_map_ref->next.get();
        if(next && next->ptr == tuple_map) {
            tuple_map_ref->next.swap(next->next);
            break;
        }
        tuple_map_ref = next;
    }
}
void ecsWorld::_findTreeRelations (entity_id eid, ecsTupleMapBase* tuple_map, ecsTupleBase* tuple) {
    assert(!tuple->parent);
    assert(!tuple->first_child);

    entity_id pid = getParent(eid);
    if(pid != NULL_ENTITY) {
        auto parent_entity = entities.deref(pid);
        auto parent_tuple  = parent_entity->findTupleOfSameContainer(tuple_map);
        if(parent_tuple) {
            parent_tuple->_addChild(tuple);
        }
    }
    
    entity_id cid = getFirstChild(eid);
    while(cid != NULL_ENTITY) {
        auto child_entity = entities.deref(cid);
        auto child_tuple  = child_entity->findTupleOfSameContainer(tuple_map);
        if(child_tuple) {
            tuple->_addChild(child_tuple);
        }

        cid = getNextSibling(cid);
    }
/*
    auto parent_id = getParent(e);
    if(parent_id != NULL_ENTITY && tuple->parent == 0) {
        auto parent = entities.deref(parent_id);
        auto parent_tuple = parent->findTupleOfSameContainer(tuple_map);
        if(parent_tuple) {
            tuple->parent = parent_tuple;
            auto child = parent_tuple->first_child;
            if (!child) {
                parent_tuple->first_child = tuple;
            } else {
                while(child != 0) {
                    if(child == tuple) {
                        break; // Already added as a child
                    }
                    auto next = child->next_sibling;
                    if(next == 0) {
                        child->next_sibling = tuple;
                        tuple->prev_sibling = child;
                        break;
                    }
                    child = next;
                }
            }
        }
    }

    auto child_id = getFirstChild(e);
    auto attachment_tuple = tuple;
    while (child_id != NULL_ENTITY) {
        auto c = entities.deref(child_id);
        auto c_tuple = c->findTupleOfSameContainer(tuple_map);
        if(!c_tuple) {
            child_id = getNextSibling(child_id);
            continue;
        }
        if(attachment_tuple == tuple) {
            c_tuple->prev_sibling = 0;
            attachment_tuple->first_child = c_tuple;
            attachment_tuple = c_tuple;
            c_tuple->parent = tuple;
        } else {
            c_tuple->prev_sibling = attachment_tuple;
            attachment_tuple->next_sibling = c_tuple;
            attachment_tuple = c_tuple;
            c_tuple->parent = tuple;
        }
        child_id = getNextSibling(child_id);
    }*/
}


ecsWorld::ecsWorld() {
    pool_index = acquireWorldIndex(this);

    global_attrib_counters.resize(get_last_attrib_id() + 1);
}

ecsWorld::~ecsWorld() {
    auto entity_set_copy = live_entities;
    for(auto& e : entity_set_copy) {
        removeEntity(e);
    }

    for(auto& kv : template_to_entities) {
        kv.first->eraseReferencingWorld(this);
    }

    freeWorldIndex(pool_index);
}

void ecsWorld::clearEntities (void) {
    auto le_copy = live_entities;
    for(auto e : le_copy) {
        removeEntity(e);
    }
}
void ecsWorld::clearSystems (void) {
    sys_by_type.clear();
    systems.clear();
}

ecsEntityHandle ecsWorld::createEntity() {
    entity_id id = entities.acquire();
    live_entities.insert(id);
    return ecsEntityHandle(this, id);
}
ecsEntityHandle ecsWorld::createEntity(archetype_mask_t attrib_signature) {
    auto hdl = createEntity();
    for(int i = 0; i < 64; ++i) {
        if(attrib_signature & (1ULL << i)) {
            createAttrib(hdl.getId(), i);
        }
    }
    return hdl;
}
ecsEntityHandle ecsWorld::createEntityFromTemplate (const char* tplPath) {
    auto tpl = retrieve<EntityTemplate>(tplPath);
    if(!tpl) {
        return ecsEntityHandle(0, -1);
    }

    ecsEntityHandle hdl = createEntity();
    dstream strm;
    tpl->serialize(strm);
    strm.jump(0);
    
    ecsWorldReadCtx ctx(this, &strm);
    ecsWorld::deserializeAttribDesc(ctx);
    ecsWorld::deserializeEntity(ctx, hdl.getId());

    uint64_t bitmaskInheritedAttribs = tpl->getEntity().getAttribBitmask();
    bitmaskInheritedAttribs &= ~(1ULL << getEcsAttribTypeLib().get_attrib_id("Translation"));
    bitmaskInheritedAttribs &= ~(1ULL << getEcsAttribTypeLib().get_attrib_id("Name"));

    // Mark all attributes as derived
    entities.deref(hdl.getId())->bitmaskInheritedAttribs = bitmaskInheritedAttribs;

    // Keep track of inherited entities
    setTemplateLink(hdl.getId(), tpl);
    auto translation = hdl.findAttrib<ecsTranslation>();
    if(translation) {
        // We don't need translation inheried from the template
        translation->setPosition(0,0,0);
    }

    return hdl;
}
void ecsWorld::removeEntity(entity_id id) {
    auto e = entities.deref(id);
    assert(e);
    assert(e->storage);

    if (e->storage) {
        auto sig = e->storage->signature;
        auto old_storage = e->storage;
        auto old_storage_index = e->storage_index;

        onAttribsRemoved(id, sig, sig);

        for (int i = 0; i < get_last_attrib_id() + 1; ++i) {
            if ((1ULL << i) & sig) {
                global_attrib_counters[i]--;
                if (global_attrib_counters[i] == 0) {
                    global_attrib_mask &= ~(1ULL << i);
                }
            }
        }

        deleteEntityData(old_storage->storage.deref(old_storage_index), sig);
        auto last_index = old_storage->storage.count() - 1;
        if (last_index != old_storage_index) {
            copyConstructEntityData(old_storage->storage.deref(old_storage_index), sig, old_storage->storage.deref(last_index), sig, this, id);
            entity_id last_ent = old_storage->storage_to_entity.back();
            entities.deref(last_ent)->storage_index = old_storage_index;
            old_storage->storage_to_entity[old_storage_index] = last_ent;
        }
        deleteEntityData(old_storage->storage.deref(last_index), sig);
        old_storage->storage.erase();
        old_storage->storage_to_entity.pop_back();

        e->storage = 0;
        e->storage_index = -1;
    }

    clearTemplateLink(id);

    auto parent_id = getParent(id);
    if(parent_id != NULL_ENTITY) {
        setParent(NULL_ENTITY, id);
    }
    auto child = getFirstChild(id);
    while(child != NULL_ENTITY) {
        auto c = child;
        child = getNextSibling(child);
        setParent(parent_id, c);
    }

    *e = ecsEntity();
    entities.free(id);
    live_entities.erase(id);
}
void ecsWorld::removeTree(entity_id id) {
    std::function<void(ecsWorld*, entity_id)> delete_fn = [&delete_fn](ecsWorld* w, entity_id e){
        auto child = w->getFirstChild(e);
        while(child != NULL_ENTITY) {
            auto next_child = w->getNextSibling(child);
            delete_fn(w, child);
            child = next_child;
        }
        w->removeEntity(e);
    };
    delete_fn(this, id);
}
const std::set<entity_id>& ecsWorld::getEntities() const {
    return live_entities;
}
ecsEntityHandle ecsWorld::findEntity (const char* name) {
    for(auto e : live_entities) {
        auto n = findAttrib<ecsName>(e);
        if(!n) continue;
        if(n->name.compare(name) == 0) {
            return ecsEntityHandle(this, e);
        }
    }
    return ecsEntityHandle(0, -1);
}
ecsEntityHandle ecsWorld::findChild (entity_id parent, const char* name) {
    auto child = getFirstChild(parent);
    while(child != NULL_ENTITY) {
        auto a = findAttrib<ecsName>(child);
        if(a) {
            if(a->name == name) {
                return ecsEntityHandle(this, child);
            }
        }
        auto hdl = findChild(child, name);
        if(hdl.isValid()) {
            return hdl;
        }
        child = getNextSibling(child);
    }
    return ecsEntityHandle(0, -1);
}
void ecsWorld::setParent (entity_id parent, entity_id child) {
    auto e = entities.deref(child);
    assert(e);

    entity_id old_parent = getParent(child);
    if (old_parent == parent) {
        return;
    }

    // Update tree depth indices
    std::stack<entity_id> stack;
    stack.push(child);
    while(!stack.empty()) {
        entity_id eid = stack.top();
        stack.pop();
        entity_id pid = getParent(eid);
        int parent_tree_depth = 0;
        if(pid != NULL_ENTITY) {
            parent_tree_depth = entities.deref(pid)->tree_depth;
        }
        entities.deref(eid)->tree_depth = parent_tree_depth + 1;

        auto c = getFirstChild(eid);
        while(c != NULL_ENTITY) {
            stack.push(c);
            c = getNextSibling(c);
        }
    }
    //

    ecsEntity* old_parent_e = 0;
    if (old_parent != NULL_ENTITY) {
        old_parent_e = entities.deref(old_parent);
    }
    if(old_parent != NULL_ENTITY && old_parent_e) {
        entity_id sib = getFirstChild(old_parent);
        if(sib != NULL_ENTITY) {
            if(sib == child) {
                old_parent_e->first_child_uid = getNextSibling(sib);
            } else {
                entity_id prev_sib = sib;
                sib = getNextSibling(sib);
                while(sib != NULL_ENTITY) {
                    if(sib == child) {
                        entity_id next_sib = getNextSibling(sib);
                        auto e0 = entities.deref(prev_sib);
                        e0->next_sibling_uid = next_sib;
                        break;
                    } else {
                        prev_sib = sib;
                        sib = getNextSibling(sib);
                    }
                }
            }
        }
        entities.deref(child)->next_sibling_uid = NULL_ENTITY;
        entities.deref(child)->parent_uid = NULL_ENTITY;
    }

    if (parent != NULL_ENTITY) {
        auto parent_e = entities.deref(parent);
        if(parent_e) {
            entity_id first_child = getFirstChild(parent);
            if(first_child == NULL_ENTITY) {
                parent_e->first_child_uid = child;
            } else { // Find last child
                entity_id last_sib = first_child;
                entity_id next_sib = getNextSibling(first_child);
                while(next_sib != NULL_ENTITY) {
                    last_sib = next_sib;
                    next_sib = getNextSibling(last_sib);
                }
                auto last_sib_e = entities.deref(last_sib);
                last_sib_e->next_sibling_uid = child;
            }
            auto child_e = entities.deref(child);
            child_e->parent_uid = parent;
            child_e->next_sibling_uid = NULL_ENTITY;
        }
    }
    
    // Update tuple relations. Tree relations turned out to be quite heavy
    if(e->storage) {
        auto tuple_map = e->first_tuple_map.get();
        while(tuple_map != 0) {
            unlinkTupleTreeRelations(tuple_map->tuple);
            _findTreeRelations(child, tuple_map->ptr, tuple_map->tuple);
            recursiveTupleMarkDirty(tuple_map->ptr, tuple_map->tuple, e->storage->signature);
            tuple_map = tuple_map->next.get();
        }
    }
}
entity_id ecsWorld::getParent (entity_id e) {
    auto en = entities.deref(e);
    assert(en);
    return en->parent_uid;
}
entity_id ecsWorld::getFirstChild (entity_id e) {
    auto en = entities.deref(e);
    assert(en);
    return en->first_child_uid;
}
entity_id ecsWorld::getNextSibling (entity_id e) {
    auto en = entities.deref(e);
    assert(en);
    return en->next_sibling_uid;
}
int ecsWorld::getTreeDepth (entity_id e) {
    auto en = entities.deref(e);
    assert(en);
    return en->tree_depth;
}


ecsEntityHandle ecsWorld::mergeWorld (const char* res_name) {
    auto w = getResource<ecsWorld>(res_name);
    if(!w) {
        return ecsEntityHandle(0,-1);
    }
    auto hdl = mergeWorld(w.get());
    hdl.getAttrib<ecsName>()->name = res_name;
    return hdl;
}
ecsEntityHandle ecsWorld::mergeWorld (ecsWorld* world) {
    auto merge_root = createEntity();

    dstream strm;
    world->serialize(strm);
    strm.jump(0);
    deserialize(strm, strm.bytes_available(), merge_root.getId());
    return merge_root;
}

ecsAttribBase* ecsWorld::findAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    if(!e->storage) {
        return 0;
    }
    uint8_t* base_ptr = (uint8_t*)e->storage->storage.deref(e->storage_index);
    auto attrib_offset = e->storage->attrib_offsets[attrib];
    if(attrib_offset == -1) {
        return 0;
    }
    return (ecsAttribBase*)(base_ptr + attrib_offset);
}
void ecsWorld::createAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);

    global_attrib_counters[attrib]++;
    global_attrib_mask |= (1ULL << attrib);

    // New stuff
    // ------------
    auto old_storage = e->storage;
    int old_storage_index = 0;
    uint64_t old_sig = 0;
    if(old_storage) {
        old_storage_index = e->storage_index;
        old_sig = old_storage->signature;
    }
    uint64_t sig = old_sig | (1ULL << attrib);
    auto storage = getArchStorage(sig);
    uint64_t index = storage->storage.alloc();
    e->storage = storage;
    e->storage_index = index;
    storage->storage_to_entity.push_back(ent);
    if(!old_storage) {
        constructEntityData(storage->storage.deref(index), sig, this, ent);
    } else {
        copyConstructEntityData(storage->storage.deref(index), sig, old_storage->storage.deref(old_storage_index), old_sig, this, ent);
        deleteEntityData(old_storage->storage.deref(old_storage_index), old_sig);
        // Move last element up
        auto last_index = old_storage->storage.count() - 1;
        if(last_index != old_storage_index) {
            copyConstructEntityData(old_storage->storage.deref(old_storage_index), old_sig, old_storage->storage.deref(last_index), old_sig, this, ent);
            entity_id last_ent = old_storage->storage_to_entity.back();
            entities.deref(last_ent)->storage_index = old_storage_index;
            old_storage->storage_to_entity[old_storage_index] = last_ent;
        }
        deleteEntityData(old_storage->storage.deref(last_index), old_sig);
        old_storage->storage.erase();
        old_storage->storage_to_entity.pop_back();
    }
    // ------------
    onAttribsCreated(ent, sig, 1ULL << attrib);
}
void ecsWorld::setAttribInheritanceMask(entity_id e, uint64_t mask) {
    auto ent = entities.deref(e);
    assert(ent);
    ent->bitmaskInheritedAttribs = mask;
}
void ecsWorld::clearAttribInheritance(entity_id e, attrib_id attrib) {
    auto ent = entities.deref(e);
    assert(ent);
    if(ent->bitmaskInheritedAttribs & (1ULL << attrib)) {
        ent->bitmaskInheritedAttribs &= ~(1ULL << attrib);
    }
    tryClearTemplateLink(e);
}
void ecsWorld::clearAllAttribInheritance(entity_id e) {
    auto ent = entities.deref(e);
    assert(ent);
    ent->bitmaskInheritedAttribs = 0;
    clearTemplateLink(e);
}
void ecsWorld::removeAttrib(entity_id ent, attrib_id attrib) {
    auto e = entities.deref(ent);
    // New stuff
    // ------------
    auto old_storage = e->storage;
    assert(old_storage);
    uint64_t old_sig = old_storage->signature;
    uint64_t sig = old_sig & ~(1ULL << attrib);
    auto storage     = getArchStorage(sig);
    auto old_storage_index = e->storage_index;

    onAttribsRemoved(ent, old_sig, 1ULL << attrib);

    if(sig != 0) {
        auto new_idx = storage->storage.alloc();
        void* dest = storage->storage.deref(new_idx);
        copyConstructEntityData(dest, sig, old_storage->storage.deref(old_storage_index), old_sig, this, ent);
        e->storage = storage;
        e->storage_index = new_idx;
        storage->storage_to_entity.push_back(ent);
    } else {
        e->storage = 0;
        e->storage_index = -1;
    }
    deleteEntityData(old_storage->storage.deref(old_storage_index), old_sig);
    auto last_index = old_storage->storage.count() - 1;
    if(last_index != old_storage_index) {
        copyConstructEntityData(old_storage->storage.deref(old_storage_index), old_sig, old_storage->storage.deref(last_index), old_sig, this, ent);
        entity_id last_ent = old_storage->storage_to_entity.back();
        entities.deref(last_ent)->storage_index = old_storage_index;
        old_storage->storage_to_entity[old_storage_index] = last_ent;
    }
    deleteEntityData(old_storage->storage.deref(last_index), old_sig);
    old_storage->storage.erase();
    old_storage->storage_to_entity.pop_back();
    // ------------

    global_attrib_counters[attrib]--;
    if(global_attrib_counters[attrib] == 0) {
        global_attrib_mask &= ~(1ULL << attrib);
    }
}
void ecsWorld::removeAttribs(entity_id ent, uint64_t mask) {
    for(int i = 0; i < get_last_attrib_id() + 1; ++i) {
        if(mask & (1ULL << i)) {
            removeAttrib(ent, i);
        }
    }
}


ecsAttribBase* ecsWorld::getAttribPtr(entity_id ent, attrib_id id) {
    return findAttrib(ent, id);
}

void ecsWorld::copyAttribs (entity_id dst, ecsEntityHandle src, uint64_t ignore_mask) {
    dstream strm;
    ecsWorldWriteCtx wctx(src.getWorld(), &strm);
    ecsWorld::serializeAttribDesc(wctx);
    ecsWorld::serializeEntity(wctx, src.getId());
    strm.jump(0);

    ecsWorldReadCtx rctx(this, &strm);
    ecsWorld::deserializeAttribDesc(rctx);
    ecsWorld::deserializeEntity(rctx, dst, ignore_mask);
}

uint64_t ecsWorld::getAttribBitmask(entity_id e) {
    auto ent = entities.deref(e);
    if(!ent) {
        assert(false);
        return 0;
    }
    if (ent->storage == 0) {
        return 0;
    }
    return ent->storage->signature;
}

uint64_t ecsWorld::getInheritedAttribBitmask(entity_id e) {
    auto ent = entities.deref(e);
    if(!ent) {
        assert(false);
        return 0;
    }
    return ent->bitmaskInheritedAttribs;
}

EntityTemplate* ecsWorld::updateTemplate(entity_id ent) {
    auto it = entity_to_template.find(ent);
    if(it == entity_to_template.end()) {
        LOG_WARN("Attempted to update template of an entity without template");
        return 0;
    }

    it->second->update(ecsEntityHandle(this, ent));

    it->second->updateDerivedEntities();

    return it->second.get();
}

void ecsWorld::updateDerived(std::shared_ptr<EntityTemplate> tpl) {
    dstream strm;
    tpl->serialize(strm);

    auto it2 = template_to_entities.find(tpl);
    for(auto e : it2->second) {
        strm.jump(0);

        uint64_t ignore_mask = ~getInheritedAttribBitmask(e);

        ecsWorldReadCtx rctx(this, &strm);
        ecsWorld::deserializeAttribDesc(rctx);
        ecsWorld::deserializeEntity(rctx, e, ignore_mask);
    }
}

void ecsWorld::linkToTemplate(entity_id ent, std::shared_ptr<EntityTemplate> tpl) {
    uint64_t ignore_mask = 0;
    ignore_mask |= (1ULL << getEcsAttribTypeLib().get_attrib_id("Translation"));
    ignore_mask |= (1ULL << getEcsAttribTypeLib().get_attrib_id("Name"));
    copyAttribs(ent, tpl->getEntity(), ignore_mask);
    uint64_t inheritanceMask = tpl->getEntity().getAttribBitmask();
    inheritanceMask &= ~ignore_mask;
    setAttribInheritanceMask(ent, inheritanceMask);
    setTemplateLink(ent, tpl);
}

void ecsWorld::resetToTemplate(entity_id ent) {
    auto it = entity_to_template.find(ent);
    if(it == entity_to_template.end()) {
        LOG_WARN("Attempted to reset an entity to template without template link");
        return;
    }

    // Do not clear Translation and Name
    uint64_t remove_mask = 0xffffffffffffffff;
    remove_mask &= ~(1ULL << getEcsAttribTypeLib().get_attrib_id("Translation"));
    remove_mask &= ~(1ULL << getEcsAttribTypeLib().get_attrib_id("Name"));
    uint64_t ignore_mask = ~remove_mask;
    
    removeAttribs(ent, remove_mask);
    copyAttribs(ent, it->second->getEntity(), ignore_mask);
    setAttribInheritanceMask(ent, it->second->getEntity().getAttribBitmask() & ~ignore_mask);
}


void ecsWorld::signalAttribUpdate(entity_id ent, attrib_id attrib) {
    uint64_t attr_mask = 1ULL << attrib;
    auto e = entities.deref(ent);
    e->signalAttribUpdate(this, attrib);/*
    if(e->getAttribBits() & attr_mask) {
        for(auto& sys : systems) {
            sys->signalUpdate(ent, attr_mask);
        }
    }*/
}

int ecsWorld::systemCount() {
    return (int)systems.size();
}
ecsSystemBase*  ecsWorld::getSystem(int i) {
    return systems[i].get();
}

void ecsWorld::update() {
    timer t;
    t.start();

    for(auto& kv : storages) {
        kv.second->onUpdateBegin();
    }

    for(auto& sys : systems) {
        sys->onUpdate();
    }

    for(auto& kv : storages) {
        kv.second->onUpdateEnd();
    }

    //LOG("ELAPSED: " << t.stop());
}


#include "util/write_ctx.hpp"
#include "util/read_ctx.hpp"

void ecsWorld::serialize(out_stream& out) {
    ecsWorldWriteCtx ctx(this, &out);

    serializeAttribDesc(ctx);

    entity_id next_new_id = 0;
    for(auto it = live_entities.begin(); it != live_entities.end(); ++it) {
        ctx.remapEntityId((*it), next_new_id++);
    }

    // Save template references
    ctx.write<uint32_t>(entity_to_template.size());
    for(auto& p : entity_to_template) {
        ctx.write<uint64_t>(ctx.getRemappedEntityId(p.first));
        ctx.writeResource(p.second);
    }

    // Entities
    ctx.write<uint64_t>(live_entities.size());
    for(auto& e : live_entities) {
        serializeEntity(ctx, e, true);
    }
}
bool ecsWorld::deserialize(in_stream& in, size_t sz) {
    return deserialize(in, sz, NULL_ENTITY);
}
bool ecsWorld::deserialize(in_stream& in, size_t sz, entity_id merge_parent) {
    ecsWorldReadCtx ctx(this, &in);

    deserializeAttribDesc(ctx);

    // Template references
    uint32_t template_ref_count = ctx.read<uint32_t>();
    std::vector<entity_id> tpl_entities;
    std::vector<std::shared_ptr<EntityTemplate>> tpl_templates;
    for(int i = 0; i < template_ref_count; ++i) {
        entity_id e = ctx.read<uint64_t>();
        auto tpl = ctx.readResource<EntityTemplate>();
        tpl_entities.push_back(e);
        tpl_templates.push_back(tpl);
    }

    // Entities
    uint64_t ent_count = ctx.read<uint64_t>();
    std::vector<entity_id> loaded_entities;
    for(int i = 0; i < ent_count; ++i) {
        auto hdl = createEntity();
        loaded_entities.push_back(hdl.getId());
        ctx.remapEntityId(i, hdl.getId());
    }

    for(int i = 0; i < tpl_entities.size() && i < tpl_templates.size(); ++i) {
        setTemplateLink(ctx.getRemappedEntityId(tpl_entities[i]), tpl_templates[i]);
    }

    for(int i = 0; i < ent_count; ++i) {
        deserializeEntity(ctx, ctx.getRemappedEntityId(i));
    }

    // Update tree depth indices
    for(int i = 0; i < loaded_entities.size(); ++i) {
        std::stack<entity_id> stack;
        stack.push(loaded_entities[i]);

        while(!stack.empty()) {
            entity_id e = stack.top();
            stack.pop();

            entity_id c = getFirstChild(e);
            while(c != NULL_ENTITY) {
                entities.deref(c)->tree_depth++;
                stack.push(c);
                c = getNextSibling(c);
            }
        }
    }
    //

    if(merge_parent != NULL_ENTITY) {
        for(int i = 0; i < loaded_entities.size(); ++i) {
            auto p = getParent(loaded_entities[i]);
            if(p == NULL_ENTITY) {
                setParent(merge_parent, loaded_entities[i]);
            }
        }
    }

    return true;
}


void ecsWorld::serializeAttribDesc (ecsWorldWriteCtx& ctx) {
    uint16_t known_attrib_count = get_last_attrib_id() + 1;
    ctx.write<uint16_t>(known_attrib_count);
    for(int i = 0; i <= get_last_attrib_id(); ++i) {
        auto inf = getEcsAttribTypeLib().get_info(i);
        std::string attrib_name;
        if(inf) {
            attrib_name = inf->name;
        } else {
            LOG_ERR("Failed to get inf for attrib " << i);
            assert(false);
        }
        ctx.writeStr(attrib_name);
    }
}
void ecsWorld::deserializeAttribDesc (ecsWorldReadCtx& ctx) {
    uint16_t known_attrib_count = ctx.read<uint16_t>();
    for(uint16_t i = 0; i < known_attrib_count; ++i) {
        std::string attrib_name = ctx.readStr();
        ctx.setAttribName(i, attrib_name.c_str());
    }
}

static int countSetBits(uint64_t i)
{
    int count = 0;
    for (; i; i >>= 1) {
      count += (i & 1);
    }
    return count;
}

void ecsWorld::serializeEntity (ecsWorldWriteCtx& ctx, entity_id e, bool keep_template_link) {
    auto ent = ctx.getWorld()->entities.deref(e);
    auto storage = ent->storage;
    auto signature = 0;
    if (storage) {
        signature = storage->signature;
    }

    uint32_t attrib_count = 0;
    uint64_t inheritedBitmask = ctx.getWorld()->getInheritedAttribBitmask(e);
    if(keep_template_link) {
        attrib_count = countSetBits(signature & ~inheritedBitmask);
    } else {
        attrib_count = countSetBits(signature);
    }

    ctx.write<uint64_t>(inheritedBitmask);
    ctx.write<entity_id>(ctx.getRemappedEntityId(ent->parent_uid));
    ctx.write<entity_id>(ctx.getRemappedEntityId(ent->next_sibling_uid));
    ctx.write<entity_id>(ctx.getRemappedEntityId(ent->first_child_uid));
    ctx.write<uint32_t>(attrib_count);
    for(int i = 0; i <= get_last_attrib_id(); ++i) {
        if(keep_template_link && (inheritedBitmask & (1ULL << i))) {
            continue;
        }
        ecsAttribBase* a = ctx.getWorld()->findAttrib(e, i);
        if(!a) {
            continue;
        }

        ctx.write<uint16_t>(i);
        
        ctx.beginSubBlock();
        
        a->write(ctx);
        
        ctx.endSubBlock();
    }
}
void ecsWorld::deserializeEntity (ecsWorldReadCtx& ctx, entity_id e, uint64_t attrib_ignore_mask) {
    uint64_t inheritedBitmask = ctx.read<uint64_t>();
    inheritedBitmask = ctx.convertMask(inheritedBitmask);

    auto ent = ctx.getWorld()->entities.deref(e);
    entity_id parent_id         = ctx.getRemappedEntityId(ctx.read<entity_id>());
    entity_id next_sibling_id   = ctx.getRemappedEntityId(ctx.read<entity_id>());
    entity_id first_child_id    = ctx.getRemappedEntityId(ctx.read<entity_id>());
    // Need to call setParent here instead of setting indices directly.
    // Otherwise if this entity already has attributes (created when deserializing other entities, e.g skin data in ecsMeshes)
    // the callbacks to restructure tuple trees won't be called, which is very bad
    // TODO: Find a way to make this whole process simpler and less prone to error
    ctx.getWorld()->setParent(parent_id, e);

    uint32_t attrib_count = ctx.read<uint32_t>();

    if(inheritedBitmask) {
        auto it = ctx.getWorld()->entity_to_template.find(e);
        if(it != ctx.getWorld()->entity_to_template.end()) {
            ctx.getWorld()->copyAttribs(e, it->second->getEntity(), ~inheritedBitmask);
            ctx.getWorld()->setAttribInheritanceMask(e, inheritedBitmask);
        }
    }

    for(int j = 0; j < attrib_count; ++j) {
        attrib_id attrib_index = (attrib_id)ctx.read<uint16_t>();

        uint64_t attrib_byte_count = ctx.read<uint64_t>(); // Byte count saved by endSubBlock()
        
        auto& attrib_name = ctx.getAttribName(attrib_index);
        // TODO: if attrib name is "" or other error - skip bytes
        if (attrib_name == "WorldTransform") {
          //__debugbreak();
        }

        attrib_index = getEcsAttribTypeLib().get_attrib_id(attrib_name.c_str());

        ecsAttribBase* a = ctx.getWorld()->getAttribPtr(e, attrib_index);

        if(attrib_ignore_mask & (1ULL << attrib_index)) {
            ctx.skipBytes(attrib_byte_count);
            continue;
        }
        
        if(!a) {
            ctx.getWorld()->createAttrib(e, attrib_index);
            a = ctx.getWorld()->getAttribPtr(e, attrib_index);
        }
        if(a) {
            a->read(ctx);
            ctx.getWorld()->signalAttribUpdate(e, attrib_index);
        }
    }
}
