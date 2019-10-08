#ifndef KT_WORLD3D_HPP
#define KT_WORLD3D_HPP

#include <tuple>
#include <stdint.h>
#include <rttr/type>

template<typename ATTRIB_BASE_T>
class Entity {
    uint64_t id;
public:
    uint64_t                attribCount() const;
    ATTRIB_BASE_T*          getAttrib(uint64_t id);
    const ATTRIB_BASE_T*    getAttrib(uint64_t id) const;
    ATTRIB_BASE_T*          getAttrib(rttr::type t);
    const ATTRIB_BASE_T*    getAttrib(rttr::type t) const;

};

template<typename... Args> 
class ktWorldSystem {
    struct Entity {
        std::tuple<Args...> attribs;
    };
public:
};

class Transform {};
class Renderable {};
class AABB {};

class ktRenderSystem : public ktWorldSystem<Transform, Renderable, AABB> {
public:

};


class ktWorld3d {
    void getDrawList();
};

#endif
