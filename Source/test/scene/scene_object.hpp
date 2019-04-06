#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP

#include "../attribute/attribute.hpp"

class Scene3;
class SceneObject {
    friend Scene3;
public:
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }

    template<typename T>
    ghandle<T> getAttrib();
    handle_ getAttrib(rttr::type t);
    handle_ findAttrib(rttr::type t);
private:
    handle_ createAttrib(rttr::type t);

    Scene3* scene;
    std::string name;
    std::map<rttr::type, handle_> attribs;
};
template<typename T>
ghandle<T> SceneObject::getAttrib() {
    return getAttrib(rttr::type::get<T>());
}

#endif
