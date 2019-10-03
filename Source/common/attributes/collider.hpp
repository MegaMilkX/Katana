#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "attribute.hpp"
#include "collision_shapes.hpp"

#include "../debug_draw.hpp"

class Collider : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    Collider() {
        setShape<SphereShape_>();
    }

    void onCreate();

    void setGhost(bool b) {
        is_ghost = b;
        resetAttribute();
    }
    bool isGhost() const {
        return is_ghost;
    }

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

    template<typename S>
    void setShape() {
        shape.reset(new S());
    }
    BaseShape_* getShape() { return shape.get(); }

    void setOffset(const gfxm::vec3& offset) {
        this->offset = offset;
        getOwner()->getTransform()->dirty();
    }
    const gfxm::vec3 getOffset() const {
        return offset;
    }

    void setDebugColor(const gfxm::vec3& col) {
        debug_color = col;
        resetAttribute();
    }
    const gfxm::vec3 getDebugColor() const {
        return debug_color;
    }

    void debugDraw(DebugDraw& dd);

    bool requiresTransformCallback() const override { return true; }

    virtual void copy(const Collider& other) {

    }
    virtual void onGui();
    void write(SceneWriteCtx& w) override {
        w.write(col_group);
        w.write(col_mask);
        w.write<uint8_t>((uint8_t)is_ghost);
        w.write(offset);
        w.write(debug_color);
        w.write(shape->get_type().get_name().to_string());
        shape->serialize(*w.strm);
    }
    void read(SceneReadCtx& r) override {
        r.read(col_group);
        r.read(col_mask);
        is_ghost = (bool)r.read<uint8_t>();
        r.read(offset);
        r.read(debug_color);
        
        std::string typestr = r.readStr();
        rttr::type t = rttr::type::get_by_name(typestr);
        if(!t.is_valid()) {
            LOG_WARN("Invalid shape type: " << typestr);
            return;
        }
        auto v = t.create();
        if(!v.is_valid() || !v.get_type().is_pointer()) {
            LOG_WARN("Failed to create shape " << typestr);
            return;
        }
        shape.reset(v.get_value<BaseShape_*>());
        shape->deserialize(*r.strm);
        resetAttribute();
    }
private:
    bool is_ghost = false;
    uint32_t col_group = 1;
    uint32_t col_mask = 1;
    std::shared_ptr<BaseShape_> shape;
    gfxm::vec3 offset;
    gfxm::vec3 debug_color = gfxm::vec3(1,1,1);
};

#endif
