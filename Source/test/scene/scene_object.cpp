#include "scene_object.hpp"

#include "scene.hpp"

handle_ SceneObject::getAttrib(rttr::type t) {
    auto h = findAttrib(t);
    if(!h) {
        h = createAttrib(t);
    }
    return h;
}
handle_ SceneObject::findAttrib(rttr::type t) {
    auto it = attribs.find(t);
    if(it != attribs.end()) {
        return it->second;
    }
    return 0;
}

handle_ SceneObject::createAttrib(rttr::type t) {
    auto h = scene->createAttrib(t);
    if(h) {
        attribs[t] = h;
    }
    return h;
}