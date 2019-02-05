#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "../component.hpp"
#include <btBulletDynamicsCommon.h>

#include "../model.hpp"
#include "../transform.hpp"

#include "../scene_components/scene_physics_world.hpp"

#include "../scene_object.hpp"

#include "collision_shape.hpp"

class PhysicsSystem;
class Collider : public Component {
    CLONEABLE
    RTTR_ENABLE(Component)
public:
    Collider() {
        collision_object = std::shared_ptr<btCollisionObject>(
            new btCollisionObject()
        );
        shape = std::shared_ptr<btCollisionShape>(
            new btCapsuleShape(1.0f, 1.6f)
        );
        collision_object->setCollisionShape(shape.get());
    }

    ~Collider() {
        world->getBtWorld()->removeCollisionObject(collision_object.get());
    }

    void onClone(Collider* other) {
        // TODO: Make an actual copy
        shape = other->shape;
        collision_object->setCollisionShape(shape.get());
        // ===
        vertices = other->vertices;
        indices = other->indices;

        center = other->center;

        updateTransform();
        resetCollisionObject();
    }

    virtual void onCreate();

    void updateTransform() {
        gfxm::mat4 t = 
            gfxm::translate(gfxm::mat4(1.0f), transform->worldPosition() + center);

        collision_object->setWorldTransform(*(btTransform*)&t);
    }

    std::shared_ptr<btCollisionObject> collision_object;
    std::shared_ptr<btCollisionShape> shape;

    virtual void _editorGui() {
        if (ImGui::BeginCombo("Shape", "Nothing", 0)) {
            if(ImGui::Selectable("Capsule")) {

            }
            if(ImGui::Selectable("Mesh")) {
                
            }
            ImGui::EndCombo();
        }

        if(ImGui::DragFloat3("Center", (float*)&center, 0.01f)) {
            updateTransform();
        }
        if(ImGui::Button("Update transform")) {
            updateTransform();
        }
        // TODO: For basic shapes
        if(ImGui::Button("Fit to model")) {

        }
        // For triangle mesh
        if(ImGui::Button("Make from model")) {
            auto mdl = getObject()->get<Model>();
            if(mdl->mesh) {
                auto msh = mdl->mesh;
                vertices.resize(msh->mesh.getAttribDataSize(gl::POSITION) / sizeof(gfxm::vec3));
                msh->mesh.copyAttribData(gl::POSITION, vertices.data());

                indices.resize(msh->mesh.getIndexDataSize() / sizeof(uint32_t));
                msh->mesh.copyIndexData(indices.data());

                gfxm::mat4 t = getObject()->get<Transform>()->getTransform();

                for(auto& v : vertices) {
                    gfxm::vec4 v4 = gfxm::vec4(v.x, v.y, v.z, 1.0f);
                    v = t * v4;
                }

                btTriangleIndexVertexArray* indexVertexArray = new btTriangleIndexVertexArray(
                    indices.size() / 3,
                    (int32_t*)indices.data(),
                    sizeof(uint32_t) * 3,
                    vertices.size() / sizeof(gfxm::vec3),
                    (btScalar*)vertices.data(),
                    sizeof(gfxm::vec3)
                );

                shape.reset(new btBvhTriangleMeshShape(
                    indexVertexArray, true
                ));
                collision_object->setCollisionShape(shape.get());

                updateTransform();
                resetCollisionObject();

                LOG("Made collision shape from model data");
            }
        }
    }
private:
    void resetCollisionObject() {
        world->getBtWorld()->removeCollisionObject(collision_object.get());
        world->getBtWorld()->addCollisionObject(collision_object.get());
        world->getBtWorld()->updateAabbs();
    }

    std::shared_ptr<CollisionShape> collision_shape;

    gfxm::vec3 center;

    Transform* transform = 0;
    PhysicsWorld* world = 0;
    PhysicsSystem* sys = 0;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> indices;
};
STATIC_RUN(Collider)
{
    rttr::registration::class_<Collider>("Collider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
