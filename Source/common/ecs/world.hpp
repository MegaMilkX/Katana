#ifndef ECS_WORLD__HPP
#define ECS_WORLD__HPP

#include "system.hpp"

#include "../util/object_pool.hpp"

#include <memory>

#include "../util/timer.hpp"

#include "entity_handle.hpp"

typedef uint64_t archetype_mask_t;

class ecsWorld {
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


    void onGuiNodeTree();
};

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
