#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "../util/serialization.hpp"
#include "base_collision_object.hpp"

#include "../scene_object.hpp"
#include "../model.hpp"

template<typename COLLISION_SHAPE_T>
class Collider : public CollisionObject<btCollisionObject, COLLISION_SHAPE_T> {
    CLONEABLE
    RTTR_ENABLE(CollisionObject<btCollisionObject, COLLISION_SHAPE_T>)
public:
    Collider() {
        bt_object->setCollisionFlags(
            bt_object->getCollisionFlags() |
            btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        bt_object->setCustomDebugColor(btVector3(.2f, 1.0f, 0.25f));
    }

    virtual void serialize(std::ostream& out) {}
    virtual void deserialize(std::istream& in, size_t sz) {}
};

class MeshCollider : public Component {
    CLONEABLE
    RTTR_ENABLE(Component)
public:
    MeshCollider() {
        bt_empty_shape.reset(new btEmptyShape());

        bt_object.reset(new btCollisionObject());
        bt_object->setCollisionShape(bt_empty_shape.get());

        bt_object->setCollisionFlags(
            bt_object->getCollisionFlags() |
            btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        bt_object->setCustomDebugColor(btVector3(.2f, 1.0f, 0.25f));
    }
    ~MeshCollider() {
        world->getBtWorld()->removeCollisionObject(bt_object.get());
    }
    virtual void onCreate() {
        bt_object->setUserPointer(getObject());

        world = getScene()->getSceneComponent<PhysicsWorld>();
        world->getBtWorld()->addCollisionObject(bt_object.get());
        transform = getObject()->get<Transform>();

        updateTransform();
    }
    void onClone(MeshCollider* other) {
        if(other->vertices.empty() || other->indices.empty()) {
            return;
        }
        vertices = other->vertices;
        indices = other->indices;
        makeMesh();
    }

    void updateTransform() {
        gfxm::mat4 t = 
            gfxm::translate(gfxm::mat4(1.0f), transform->worldPosition() + center);
        bt_object->setWorldTransform(*(btTransform*)&t);
    }

    void makeFromModel(Model* mdl) {
        /*
        if(mdl->mesh) {
            vertices.clear();
            indices.clear();

            auto msh = mdl->mesh;
            vertices.resize(msh->mesh.getAttribDataSize(gl::POSITION) / sizeof(gfxm::vec3));
            msh->mesh.copyAttribData(gl::POSITION, vertices.data());

            indices.resize(msh->mesh.getIndexDataSize() / sizeof(uint32_t));
            msh->mesh.copyIndexData(indices.data());

            gfxm::mat4 t = 
                gfxm::to_mat4(
                    gfxm::to_mat3(mdl->getObject()->get<Transform>()->getTransform())
                );
            for(auto& v : vertices) {
                gfxm::vec4 v4 = gfxm::vec4(v.x, v.y, v.z, 1.0f);
                v = t * v4;
            }

            makeMesh();
        }*/
    }

    virtual void _editorGui() {
        if(ImGui::Button("Make from model")) {
            makeFromModel(getObject()->get<Model>());            
        }
        updateTransform();
    }

    virtual void serialize(std::ostream& out) {
        write(out, group_mask);

        write(out, center);
        
        write(out, (uint64_t)vertices.size());
        out.write((char*)vertices.data(), sizeof(gfxm::vec3) * vertices.size());
        write(out, (uint64_t)indices.size());
        out.write((char*)indices.data(), sizeof(uint32_t) * indices.size());
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        group_mask = read<uint64_t>(in);
        
        center = read<gfxm::vec3>(in);

        uint64_t v_count = read<uint64_t>(in);
        vertices.resize(v_count);
        in.read((char*)vertices.data(), v_count * sizeof(gfxm::vec3));
        uint64_t i_count = read<uint64_t>(in);
        indices.resize(i_count);
        in.read((char*)indices.data(), i_count * sizeof(uint32_t));

        makeMesh();
    }
private:
    void makeMesh() {
        indexVertexArray.reset(new btTriangleIndexVertexArray(
            indices.size() / 3,
            (int32_t*)indices.data(),
            sizeof(uint32_t) * 3,
            vertices.size() / sizeof(gfxm::vec3),
            (btScalar*)vertices.data(),
            sizeof(gfxm::vec3)
        ));
        bt_mesh_shape.reset(new btBvhTriangleMeshShape(
            indexVertexArray.get(), true
        ));
        bt_object->setCollisionShape(bt_mesh_shape.get());
    }

    uint64_t group_mask = 0;
    gfxm::vec3 center;

    std::shared_ptr<btCollisionObject> bt_object;
    std::shared_ptr<btBvhTriangleMeshShape> bt_mesh_shape;
    std::shared_ptr<btEmptyShape> bt_empty_shape;
    std::shared_ptr<btTriangleIndexVertexArray> indexVertexArray;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> indices;
    
    PhysicsWorld* world = 0;
    Transform* transform = 0;
};

typedef Collider<Collision::Box> BoxCollider;
typedef Collider<Collision::Sphere> SphereCollider;
typedef Collider<Collision::Cylinder> CylinderCollider;
typedef Collider<Collision::Capsule> CapsuleCollider;

STATIC_RUN(Colliders)
{
    rttr::registration::class_<BoxCollider>("BoxCollider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<SphereCollider>("SphereCollider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<CylinderCollider>("CylinderCollider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
    rttr::registration::class_<CapsuleCollider>("CapsuleCollider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );

    rttr::registration::class_<MeshCollider>("MeshCollider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif