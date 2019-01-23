#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP

#include "component.hpp"

#include <map>
#include <string>

class Scene;
class SceneObject {
    friend Scene;
public:
    SceneObject(Scene* scene, SceneObject* parent)
    : scene(scene), parent(parent) {}

    void            destroy();

    size_t          getId() const;
    void            setName(const std::string& name);
    std::string     getName() const;
    template<typename T>
    T*              get();
    Component*      get(const std::string& type);
    template<typename T>
    T*              find();
    Component*      find(const std::string& type);
    void            removeComponents();
    SceneObject*    createChild();
    size_t          childCount();
    SceneObject*    getChild(size_t index);
    void            removeChild(SceneObject* so);
    SceneObject*    getParent();
    void            setParent(SceneObject* so);

    void            cloneFrom(SceneObject* other);

    SceneObject*    findObject(const std::string& name);

    Scene*          getScene();

    void            serialize(std::ostream& out);
    void            deserialize(std::istream& in);

    void            _editorGui();
private:
    size_t id;
    std::string name;
    Scene* scene;
    SceneObject* parent;
    std::vector<SceneObject*> children;
    std::map<rttr::type, Component*> components;
};

template<typename T>
T* SceneObject::get() {
    if(!scene) return 0;
    T* c = find<T>();
    if(!c) {
        c = scene->createComponent<T>(this);
        components[rttr::type::get<T>()] = c;
    } 
    return c;
}

template<typename T>
T* SceneObject::find() {
    if(!scene) return 0;
    if(components.count(rttr::type::get<T>()) == 0) {
        return 0;
    }
    return (T*)components[rttr::type::get<T>()];
}

#endif
