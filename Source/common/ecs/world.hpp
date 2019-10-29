#ifndef ECS_WORLD__HPP
#define ECS_WORLD__HPP

#include "system.hpp"

#include "../util/object_pool.hpp"

#include <memory>

class timer
{
public:
    timer()
    {
        QueryPerformanceFrequency(&freq);
    }

    void start()
    {
        QueryPerformanceCounter(&_start);
    }
    
    int64_t stop()
    {
        QueryPerformanceCounter(&_end);
        elapsed.QuadPart = _end.QuadPart - _start.QuadPart;
        elapsed.QuadPart *= 1000000;
        elapsed.QuadPart /= freq.QuadPart;
        return elapsed.QuadPart;
    }
private:
    LARGE_INTEGER freq;
    LARGE_INTEGER _start, _end;
    LARGE_INTEGER elapsed;
};

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

    template<typename T>
    T* findAttrib(entity_id ent) {
        auto e = entities.deref(ent);
        auto a = e->findAttrib<T>();
        return a;
    }

    ecsAttribBase* getAttribPtr(entity_id ent, attrib_id id) {
        auto e = entities.deref(ent);
        if(!e) return 0;
        return e->getAttribPtr(id);
    }

    template<typename T>
    void signalAttribUpdate(entity_id ent) {
        uint64_t attr_mask = 1 << T::get_id_static();
        auto e = entities.deref(ent);
        if(e->getAttribBits() & attr_mask) {
            for(auto& sys : systems) {
                sys->signalUpdate(ent, 1 << T::get_id_static());
            }
        }
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
        timer t;
        t.start();
        for(auto& sys : systems) {
            sys->onUpdate();
        }
        LOG("ELAPSED: " << t.stop());
    }
};

#endif
