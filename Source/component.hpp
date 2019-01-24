#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include "lib/imgui_wrap.hpp"
#include <type_traits>

#include <rttr/registration>
#include <rttr/registration_friend>
#include <rttr/type>

#include "util/static_run.h"

#define CLONEABLE \
public: \
    Component* clone() { \
        return new std::remove_reference<decltype(*this)>::type(*this); \
    }

class SceneObject;
class Scene;
class Component {
    RTTR_ENABLE()

    friend Scene;
    friend SceneObject;
public:
    Component() : scene_object(0) {}
    virtual ~Component() {}
    SceneObject* getObject() {
        return scene_object;
    }
    uint64_t getId() { return id; }

    virtual void onCreate() {}
    virtual Component* clone() { return 0; }

    Scene* getScene();
    template<typename T>
    T*     get();

    virtual void serialize(std::ostream& out) {}
    virtual void deserialize(std::istream& in) {}

    virtual void _editorGui() {}
private:
    uint64_t id;
    SceneObject* scene_object;
};

template<typename T>
T* Component::get() {
    return scene_object->get<T>();
}

#endif
