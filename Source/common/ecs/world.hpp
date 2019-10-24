#ifndef ECS_WORLD__HPP
#define ECS_WORLD__HPP

#include "system.hpp"

#include "../util/object_pool.hpp"

#include <memory>

class ecsWorld {
    ObjectPool<ecsEntity> entities;
    std::vector<std::unique_ptr<ecsSystemBase>> systems;

public:
    entity_id createEntity() {
        return entities.acquire();
    }
    void removeEntity(entity_id id) {
        entities.free(id);
    }
    ecsEntity* getEntity(entity_id id) {
        return entities.deref(id);
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
