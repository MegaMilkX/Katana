#ifndef ATTRIBUTE_HPP
#define ATTRIBUTE_HPP

#include "../util/reg_type.hpp"

class SceneObject;
class Attribute {
    RTTR_ENABLE()
public:
    void setOwner(ghandle<SceneObject> o) { object = o; }
    ghandle<SceneObject> getOwner() { return object; }
private:
    ghandle<SceneObject> object;
};

#endif
