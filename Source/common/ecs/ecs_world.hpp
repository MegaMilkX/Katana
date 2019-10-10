#ifndef ECS_WORLD_HPP
#define ECS_WORLD_HPP

#include "entity.hpp"
#include "ecs_system.hpp"
#include "bytepool.hpp"

#include <rttr/type>
#include <unordered_map>
#include <map>
#include <memory>

class ktEcsWorld {
    std::unordered_map<
        rttr::type,
        std::shared_ptr<ktEcsISystem>
    > systems;
    std::unordered_map<
        rttr::type, 
        std::shared_ptr<ktBytePool>
    > attrib_pools;
    std::unordered_map<
        rttr::type,
        std::map<
            ktEntity,
            size_t
        >
    > attribs;

    bool poolExists(rttr::type t) {
        return attrib_pools.count(t);
    }
    void createPool(rttr::type t, size_t block_size) {
        attrib_pools[t].reset(new ktBytePool(block_size));
    }

    void signalAttribCreated(ktEntity ent, rttr::type t) {
        for(auto& kt : systems) {
            kt.second->onAttribCreated(*this, ent, t);
        }
    }
    void signalAttribRemoved(ktEntity ent, rttr::type t) {
        for(auto& kt : systems) {
            kt.second->onAttribRemoved(*this, ent, t);
        }
    }

public:
    template<typename T>
    void        addSystem();

    ktEntity    createEntity();
    template<typename T>
    void        createAttrib(ktEntity ent);
    template<typename T>
    void        removeAttrib(ktEntity ent);
    template<typename T>
    void        updateAttrib(ktEntity ent, const T& value);
    size_t      findAttrib(ktEntity ent, rttr::type t);
    template<typename... Args>
    bool        hasAttribs(ktEntity ent);
    bool        hasAttrib(ktEntity ent, rttr::type t);

    void update();
};


template<typename T>
void ktEcsWorld::addSystem() {
    systems[rttr::type::get<T>()].reset(new T());
}

ktEntity ktEcsWorld::createEntity() {
    static uint64_t id = 0;
    return ktEntity(this, id++);
}
template<typename T>
void ktEcsWorld::createAttrib(ktEntity ent) {
    if(!poolExists(rttr::type::get<T>())) {
        createPool(rttr::type::get<T>(), sizeof(T));
    }
    if(attribs[rttr::type::get<T>()].count(ent)) {
        return;
    }
    size_t attrib_id = attrib_pools[rttr::type::get<T>()]->acquire();
    attribs[rttr::type::get<T>()][ent] = attrib_id;

    signalAttribCreated(ent, rttr::type::get<T>());
}
template<typename T>
void ktEcsWorld::removeAttrib(ktEntity ent) {
    rttr::type t = rttr::type::get<T>();
    if(!poolExists(t)) {
        return;
    }
    auto it = attribs[t].find(ent);
    if(it == attribs[t].end()) {
        return;
    }
    attrib_pools[t]->release(it->second);
    attribs[t].erase(it);

    signalAttribRemoved(ent, t);
}
size_t ktEcsWorld::findAttrib(ktEntity ent, rttr::type t) {
    return 0;
}
template<typename... Args>
bool ktEcsWorld::hasAttribs(ktEntity ent) {
    static const rttr::type types[] = { rttr::type::get<Args>()... };
    for(size_t i = 0; i < sizeof(types) / sizeof(types[0]); ++i) {
        if(!hasAttrib(ent, types[i])) {
            return false;
        }
    }
    return true;
}

bool ktEcsWorld::hasAttrib(ktEntity ent, rttr::type t) {
    auto it = attribs[t].find(ent);
    if(it == attribs[t].end()) {
        return false;
    }
    return true;
}


void ktEcsWorld::update() {
    for(auto& kv : systems) {
        kv.second->onInvoke();
    }
}

#endif
