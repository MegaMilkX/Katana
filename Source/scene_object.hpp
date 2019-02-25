#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP

#include "component.hpp"

#include <map>
#include <string>

template<typename T>
class CmpHdl {
    friend Scene;
public:
    CmpHdl() {}
    CmpHdl(Scene* scene, int index) 
    : scene(scene), index(index) {}
    T* operator->() {
        if(!scene) return 0;
        return scene->components.at(rttr::type::get<T>())[index].get();
    }
private:
    Scene* scene = 0;
    int index = 0;
};

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
    template<typename T>
    void            findAllRecursive(std::vector<T*>& out);
    void            removeComponent(rttr::type t);
    void            removeComponents();
    SceneObject*    createChild();
    size_t          childCount();
    SceneObject*    getChild(size_t index);
    void            removeChild(SceneObject* so);
    SceneObject*    getParent();
    void            setParent(SceneObject* so);

    SceneObject*    getTopObject();

    void            cloneFrom(SceneObject* other);

    SceneObject*    findObject(const std::string& name);

    Scene*          getScene();

    void            serialize(std::ostream& out);
    void            deserialize(std::istream& in);

    void            _editorGui();
private:
    size_t id = 0;
    std::string name;
    Scene* scene = 0;
    SceneObject* parent = 0;
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

template<typename T>
void SceneObject::findAllRecursive(std::vector<T*>& out) {
    if(!scene) return;
    if(components.count(rttr::type::get<T>())) {
        out.emplace_back((T*)components[rttr::type::get<T>()]);
    }
    for(auto c : children) {
        c->findAllRecursive<T>(out);
    }
}

#endif
