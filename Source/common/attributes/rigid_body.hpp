#ifndef RIGID_BODY2_HPP
#define RIGID_BODY2_HPP

#include "attribute.hpp"
#include "collision_shapes.hpp"
#include <btBulletDynamicsCommon.h>

class RigidBody : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    RigidBody() {
        shape.reset(new SphereShape_());
    }

    void onCreate();

    void setCollisionGroup(uint32_t g) {
        col_group = g;
        resetAttribute();
    }
    uint32_t getCollisionGroup() const {
        return col_group;
    }
    void setCollisionMask(uint32_t m) {
        col_mask = m;
        resetAttribute();
    }
    uint32_t getCollisionMask() const {
        return col_mask;
    }

    void setMass(float m) {
        mass = m;
        resetAttribute();
    }
    float getMass() const {
        return mass;
    }

    BaseShape_* getShape() {
        return shape.get();
    }

    void copy(const RigidBody& other) {
        
    }
    virtual void onGui();
    virtual bool serialize(out_stream& out) {
        DataWriter w(&out);
        w.write(col_group);
        w.write(col_mask);
        w.write(mass);
        w.write(shape->get_type().get_name().to_string());
        shape->serialize(out);
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        DataReader r(&in);
        r.read(col_group);
        r.read(col_mask);
        r.read(mass);
        
        std::string typestr = r.readStr();
        rttr::type t = rttr::type::get_by_name(typestr);
        if(!t.is_valid()) {
            LOG_WARN("Invalid shape type: " << typestr);
            return false;
        }
        auto v = t.create();
        if(!v.is_valid() || !v.get_type().is_pointer()) {
            LOG_WARN("Failed to create shape " << typestr);
            return false;
        }
        shape.reset(v.get_value<BaseShape_*>());
        shape->deserialize(in);
        resetAttribute();
        return true;
    }
private:
    uint32_t col_group = 1;
    uint32_t col_mask = 1;
    float mass = 1.0f;
    std::shared_ptr<BaseShape_> shape;
};
REG_ATTRIB(RigidBody, RigidBody, Physics);

#endif
