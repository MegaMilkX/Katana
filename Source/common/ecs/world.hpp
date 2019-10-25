#ifndef ECS_WORLD__HPP
#define ECS_WORLD__HPP

#include "system.hpp"

#include "../util/object_pool.hpp"

#include <memory>

class ecsWorld {
    ObjectPool<ecsEntity> entities;
    std::vector<std::unique_ptr<ecsSystemBase>> systems;

    std::set<entity_id> live_entities;

public:
    entity_id createEntity() {
        entity_id id = entities.acquire();
        live_entities.insert(id);
        return id;
    }
    void removeEntity(entity_id id) {
        entities.free(id);
        live_entities.erase(id);
    }
    ecsEntity* getEntity(entity_id id) {
        return entities.deref(id);
    }

    const std::set<entity_id>& getEntities() const {
        return live_entities;
    }

    template<typename T>
    void setAttrib(entity_id ent) {
        auto e = entities.deref(ent);
        auto a = e->getAttrib<T>();
        e->setBit(a->get_id());
        for(auto& sys : systems) {
            sys->tryFit(this, ent, e->getAttribBits());
        }
    }

    ecsAttribBase* getAttribPtr(entity_id ent, attrib_id id) {
        auto e = entities.deref(ent);
        if(!e) return 0;
        return e->getAttribPtr(id);
    }

    template<typename T>
    void updateAttrib(entity_id ent, const T& value) {
        auto e = entities.deref(ent);
        if(e->updateAttrib(value)) {
            for(auto& sys : systems) {
                sys->signalUpdate(ent, 1 << T::get_id_static());
            }
        }
    }

    template<typename T>
    T* addSystem() {
        T* sys = new T();
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        return sys;
    }
    template<typename T>
    T* addSystem(T* sys) {
        systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        return sys;
    }
    void addSystem(const std::vector<ecsSystemBase*>& sys_array) {
        for(auto& sys : sys_array) {
            systems.push_back(std::unique_ptr<ecsSystemBase>(sys));
        }
    }

    void update() {
        for(auto& sys : systems) {
            sys->onUpdate();
        }
    }
};

#endif
