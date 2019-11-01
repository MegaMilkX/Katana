#ifndef ECS_TRANSFORM_HPP
#define ECS_TRANSFORM_HPP

#include "../attribute.hpp"

#include "../../gfxm.hpp"

#include <set>

class ecsysSceneGraph;

class ecsTransform : public ecsAttrib<ecsTransform> {
    friend ecsysSceneGraph;

    entity_id               entity_uid;
    bool                    _dirty = false;
    gfxm::vec3              _position = gfxm::vec3(.0f, .0f, .0f);
    gfxm::quat              _rotation = gfxm::quat(.0f, .0f, .0f, 1.0f);
    gfxm::vec3              _scale = gfxm::vec3(1.0f, 1.0f, 1.0f);
    gfxm::mat4              _world_transform = gfxm::mat4(1.0f);
    ecsTransform*           _parent = 0;
    std::set<ecsTransform*> _children;

    void setParent(ecsTransform* p);

public:
    void dirty();
    bool isDirty();

    void lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up, float f = 1.0f);

    void translate(float x, float y, float z);
    void translate(const gfxm::vec3& vec);
    void rotate(float angle, float axisX, float axisY, float axisZ);
    void rotate(float angle, const gfxm::vec3& axis);
    void rotate(const gfxm::quat& q);

    void setPosition(float x, float y, float z);
    void setPosition(const gfxm::vec3& position);
    void setRotation(float x, float y, float z);
    void setRotation(gfxm::vec3 euler);
    void setRotation(const gfxm::quat& rotation);
    void setRotation(float x, float y, float z, float w);
    void setScale(float s);
    void setScale(float x, float y, float z);
    void setScale(const gfxm::vec3& s);
    void setTransform(gfxm::mat4 t);

    const gfxm::vec3& getPosition() const;
    const gfxm::quat& getRotation() const;
    const gfxm::vec3& getScale() const;
    gfxm::vec3        getEulerAngles();
    gfxm::vec3        getWorldPosition();
    gfxm::quat        getWorldRotation();
    gfxm::vec3        getWorldScale();

    gfxm::vec3 right();
    gfxm::vec3 up();
    gfxm::vec3 back();
    gfxm::vec3 left();
    gfxm::vec3 down();
    gfxm::vec3 forward();

    gfxm::mat4        getLocalTransform() const;
    gfxm::mat4        getParentTransform();
    const gfxm::mat4& getWorldTransform();

    ecsTransform* getParent() const;

    void onGui(ecsWorld* world, entity_id ent);

};


#endif
