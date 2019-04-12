#ifndef COLLISION_SHAPE_HPP
#define COLLISION_SHAPE_HPP

#include "component.hpp"
#include "../../common/util/log.hpp"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <bulletcollision/collisiondispatch/btghostobject.h>
#include "../../common/gfxm.hpp"

#include "../../common/util/imgui_helpers.hpp"

class CollisionShape;
class BaseShape {
    RTTR_ENABLE()
public:
    virtual ~BaseShape() {}
    void setWrapper(CollisionShape* w) {
        shape_wrapper = w;
    }
    virtual void debugDraw(btIDebugDraw* dd, const gfxm::mat4& t) = 0;
    virtual void onGui() = 0;
    virtual btCollisionShape* getBtShape() = 0;

    virtual void copy(const BaseShape& other) = 0;

    virtual void serialize(out_stream& out) {}
    virtual void deserialize(in_stream& in) {}
protected:
    CollisionShape* shape_wrapper = 0;
};

class SphereShape : public BaseShape {
    RTTR_ENABLE(BaseShape)
public:
    SphereShape() {
        shape.reset(new btSphereShape(radius));
    }
    virtual void debugDraw(btIDebugDraw* dd, const gfxm::mat4& t) {
        /*
        btTransform btt;
        btt.setFromOpenGLMatrix((float*)&t);
        dd->drawSphere(1.0f, btt, btVector3(0.0f,0.8f,0.0f));
        */
    }
    virtual void onGui() {
        if(ImGui::DragFloat(MKSTR("Radius##" << shape).c_str(), (float*)&radius, 0.001f)) {
            *shape.get() = btSphereShape(radius);
        }
    }
    virtual btCollisionShape* getBtShape() {
        return shape.get();
    }
    virtual void copy(const BaseShape& other) {
        SphereShape& o = (SphereShape&)other;
        radius = o.radius;
        *shape.get() = btSphereShape(radius);
    }

    virtual void serialize(out_stream& out) {
        out.write(radius);
    }
    virtual void deserialize(in_stream& in) {
        radius = in.read<float>();
        *shape.get() = btSphereShape(radius);
    }
private:
    std::shared_ptr<btSphereShape> shape;
    float radius = .5f;
};
STATIC_RUN(SphereShape) {
    rttr::registration::class_<SphereShape>("SphereShape")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}
class BoxShape : public BaseShape {
    RTTR_ENABLE(BaseShape)
public:
    BoxShape() {
        size = gfxm::vec3(.5f, .5f, .5f);
        shape.reset(new btBoxShape(btVector3(size.x, size.y, size.z)));
    }
    virtual void debugDraw(btIDebugDraw* dd, const gfxm::mat4& t) {
        /*
        btTransform btt;
        btt.setFromOpenGLMatrix((float*)&t);
        dd->drawBox(btVector3(-.5f,-.5f,-.5f), btVector3(.5f,.5f,.5f), btt, btVector3(0.7f,0.5f,0.5f));
        */
    }
    virtual void onGui() {
        if(ImGui::DragFloat3(MKSTR("Size##" << shape).c_str(), (float*)&size, 0.001f)) {
            *shape.get() = btBoxShape(btVector3(size.x, size.y, size.z));
        }
    }
    virtual btCollisionShape* getBtShape() {
        return shape.get();
    }
    virtual void copy(const BaseShape& other) {
        BoxShape& o = (BoxShape&)other;
        size = o.size;
        *shape.get() = btBoxShape(btVector3(size.x, size.y, size.z));
    }

    virtual void serialize(out_stream& out) {
        out.write(size);
    }
    virtual void deserialize(in_stream& in) {
        size = in.read<gfxm::vec3>();
        *shape.get() = btBoxShape(btVector3(size.x, size.y, size.z));
    }
private:
    std::shared_ptr<btBoxShape> shape;
    gfxm::vec3 size;
};
STATIC_RUN(BoxShape) {
    rttr::registration::class_<BoxShape>("BoxShape")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}
class CapsuleShape : public BaseShape {
    RTTR_ENABLE(BaseShape)
public:
    CapsuleShape() {
        shape.reset(new btCapsuleShape(width, height));
    }
    virtual void debugDraw(btIDebugDraw* dd, const gfxm::mat4& t) {
        
    }
    virtual void onGui() {
        if(ImGui::DragFloat(MKSTR("Radius##" << shape).c_str(), (float*)&width, 0.001f)) {
            *shape.get() = btCapsuleShape(width, height);
        }
        if(ImGui::DragFloat(MKSTR("Height##" << shape).c_str(), (float*)&height, 0.001f)) {
            *shape.get() = btCapsuleShape(width, height);
        }
    }
    virtual btCollisionShape* getBtShape() {
        return shape.get();
    }
    virtual void copy(const BaseShape& other) {
        CapsuleShape& o = (CapsuleShape&)other;
        height = o.height;
        width = o.width;
        *shape.get() = btCapsuleShape(width, height);
    }

    virtual void serialize(out_stream& out) {
        out.write(height);
        out.write(width);
    }
    virtual void deserialize(in_stream& in) {
        height = in.read<float>();
        width = in.read<float>();
        *shape.get() = btCapsuleShape(width, height);
    }
private:
    std::shared_ptr<btCapsuleShape> shape;
    float height = 1.5f;
    float width = .5f;
};
STATIC_RUN(CapsuleShape) {
    rttr::registration::class_<CapsuleShape>("CapsuleShape")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

class Model;
class Mesh;
class MeshShape : public BaseShape {
    RTTR_ENABLE(BaseShape)
public:
    MeshShape() {
        empty.reset(new btEmptyShape());
        shape = empty.get();
    }
    virtual void debugDraw(btIDebugDraw* dd, const gfxm::mat4& t) {
        
    }
    virtual void onGui();
    virtual btCollisionShape* getBtShape() {
        return shape;
    }

    virtual void copy(const BaseShape& other) {
        MeshShape& o = (MeshShape&)other;
        setMesh(o.mesh);
    }

    virtual void serialize(out_stream& out);
    virtual void deserialize(in_stream& in);

    void makeFromModel();
    void makeFromModel(std::shared_ptr<Model> mdl);
private:
    //void makeMesh();
    void setMesh(const std::string& res_name);
    void setMesh(std::shared_ptr<Mesh> mesh);

    std::shared_ptr<Mesh> mesh;

    btCollisionShape* shape = 0;

    std::shared_ptr<btEmptyShape> empty;
    std::shared_ptr<btBvhTriangleMeshShape> bt_mesh_shape;
    std::shared_ptr<btTriangleIndexVertexArray> indexVertexArray;
};
STATIC_RUN(MeshShape) {
    rttr::registration::class_<MeshShape>("MeshShape")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

class CollisionShape : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    CollisionShape() {

    }
    ~CollisionShape();

    virtual void onCreate();

    virtual void copy(Attribute* other) {
        if(other->get_type() != get_type()) {
            LOG("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        CollisionShape* s = (CollisionShape*)other;

        if(s->shape) {
            copyShape(*s->shape.get());
        }
        offset = s->offset;
        _shapeChanged();
    }

    btCollisionShape* getBtShape() {
        if(shape) return shape->getBtShape();
        return 0;
    }

    void debugDraw(btIDebugDraw* dd);

    void setOffset(const gfxm::vec3& o) {
        offset = o;
    }
    const gfxm::vec3& getOffset() const {
        return offset;
    }

    void setShape(rttr::type t) {
        if(!t.is_derived_from(rttr::type::get<BaseShape>())) {
            return;
        }
        rttr::variant v = t.create();
        if(!v.is_valid()) {
            LOG("Failed to create shape");
            return;
        }
        shape.reset(v.get_value<BaseShape*>());
        shape->setWrapper(this);
        _shapeChanged();
    }
    rttr::type getShapeType() {
        if(shape) {
            return shape->get_type();
        } else {
            return rttr::type::get<void>();
        }
    }

    virtual bool serialize(out_stream& out) {
        DataWriter w(&out);
        if(shape) w.write(shape->get_type().get_name().to_string());
        else w.write(std::string());
        
        w.write(offset);

        if(shape) {
            shape->serialize(out);
        }
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        DataReader r(&in);
        std::string shape_type_name = r.readStr();
        if(!shape_type_name.empty()) {
            setShape(rttr::type::get_by_name(shape_type_name));
        }
        offset = r.read<gfxm::vec3>();

        if(shape) {
            shape->deserialize(in);
        }
        return true;
    }
    virtual void onGui() {
        auto derived_array = rttr::type::get<BaseShape>().get_derived_classes();
        std::string current = shape ? shape->get_type().get_name().to_string() : "<null>";
        if(ImGui::BeginCombo("shape", current.c_str())) {
            for(auto& d : derived_array) {
                if(ImGui::Selectable(d.get_name().to_string().c_str(), false)) {
                    setShape(d);
                }
            }
            ImGui::EndCombo();
        }
        if(shape) {
            shape->onGui();
        }
        if(ImGui::DragFloat3(MKSTR("Offset##" << shape).c_str(), (float*)&offset, 0.001f)) {
            _shapeChanged();
        }
    }

    void _shapeChanged();
private:
    void copyShape(const BaseShape& s) {
        auto t = s.get_type();
        if(!t.is_derived_from(rttr::type::get<BaseShape>())) {
            return;
        }
        rttr::variant v = t.create();
        if(!v.is_valid()) {
            LOG("Failed to create shape");
            return;
        }
        shape.reset(v.get_value<BaseShape*>());
        shape->setWrapper(this);
        shape->copy(s);
        _shapeChanged();
    }

    std::shared_ptr<BaseShape> shape;
    gfxm::vec3 offset;
};

#endif
