#ifndef KT_WORLD3D_HPP
#define KT_WORLD3D_HPP

#include <vector>
#include <set>
#include <tuple>
#include <stdint.h>
#include <rttr/type>
#include "gfxm.hpp"

template<typename T>
class Pool {
    std::vector<T> data;
    std::set<uint64_t> free_slots;
public:
    uint64_t acquire() {
        uint64_t id;
        if(!free_slots.empty()) {
            id = free_slots.back();
            free_slots.erase(free_slots.end());
        } else {
            id = data.size();
            data.emplace_back(T());
        }
        return id;
    }
    void free(uint64_t id) {
        free_slots.insert(id);
    }
    T* deref(uint64_t id) {
        return data[id];
    }
};

template<typename T>
class ktAttribId {
    static uint16_t getNextUid() { 
        static uint16_t s_uid = 0;
        uint16_t uid = s_uid;
        ++s_uid;
        return uid;
    }
    static const uint16_t attr_uid;
public:
    static uint16_t getAttribUid() {
        return attr_uid;
    }
};
template<typename T>
const uint16_t ktAttribId<T>::attr_uid = ktAttribId<T>::getNextUid();

class Transform : public ktAttribId<Transform> {

};

template<typename T>
class AttribMap {
    std::unordered_map<uint64_t, T> data;
public:
    T* get(uint64_t ent_id);
    T* find(uint64_t ent_id);
    void erase(uint64_t ent_id);
};


class Entity {
public:
    uint64_t id;

    size_t attribCount() const {
        return 0;
    }
};

class attrTransform {};
class attrMesh {};
class attrAABB {
    gfxm::aabb aabb;
};
class attrOmniLight {
    gfxm::vec3 color = gfxm::vec3(1.0f, 1.0f, 1.0f);
    float radius = 5.0f;
    float intensity = 5.0f;
};

template<typename... Args>
class ktArchetype {
public:
    std::tuple<Args*...> attribs;

    static bool entityFits(Entity ent) {
        size_t arg_count = sizeof...(Args);
        size_t attrib_count = ent.attribCount();
        if(arg_count > attrib_count) {
            return false;
        }

        std::tuple<Args*...> attribs;
        uint16_t x[] = { Args::attr_uid... };
        for(int i = 0; i < arg_count; ++i) {
            auto attr = ent.getAttrib(x[i]);
            if(!attr) {
                return false;
            } 
        }

        return true;
    }
};

class Renderable : public ktArchetype<attrTransform, attrMesh, attrAABB> {
public:

};

class OmniLight : public ktArchetype<attrTransform, attrOmniLight> {
public:

};

template<typename... Args> 
class ktWorldSystem {
public:
};


class ktRenderSystem : public ktWorldSystem<Renderable, OmniLight> {
public:

};


class ktWorld3d {
    void getDrawList();
};

void foo() {
    OmniLight light;
    std::get<0>(light.attribs);
    std::get<1>(light.attribs);

    AttribMap<Transform> amap;
    amap.get(Transform::getAttribUid());
}

#endif
