#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include "../component.hpp"
#include <btBulletDynamicsCommon.h>

#include "../model.hpp"
#include "../transform.hpp"

#include "../scene_components/scene_physics_world.hpp"

#include "../scene_object.hpp"

#include "collision_shape.hpp"

#include "../util/serialization.hpp"

class PhysicsSystem;
class Collider : public Component {
    CLONEABLE
    RTTR_ENABLE(Component)
public:
    Collider() {
        collision_object = std::shared_ptr<btCollisionObject>(
            new btCollisionObject()
        );
        collision_object->setUserPointer(this);
    }

    ~Collider() {
        world->getBtWorld()->removeCollisionObject(collision_object.get());
    }

    void onClone(Collider* other) {
        // TODO: Make an actual copy
        collision_shape.reset(other->collision_shape->clone());
        collision_object->setCollisionShape(collision_shape->getBtShape());
        collision_group = other->collision_group;
        collision_object->setCollisionFlags(collision_group);

        // ===
        //vertices = other->vertices;
        //indices = other->indices;

        //center = other->center;

        updateTransform();
        resetCollisionObject();
    }

    virtual void onCreate();

    unsigned getGroupMask() {
        return collision_object->getCollisionFlags();
    }

    bool contactTest(gfxm::vec3& new_pos, unsigned flags) {
        return world->contactTest(collision_object.get(), new_pos, flags);
    }

    template<typename T>
    void setShape() {
        collision_shape.reset(new T());
        collision_object->setCollisionShape(collision_shape->getBtShape());
    }

    void refreshShape() {
        collision_object->setCollisionShape(collision_shape->getBtShape());
    }

    void updateTransform() {
        gfxm::mat4 t = 
            gfxm::translate(gfxm::mat4(1.0f), transform->worldPosition() + collision_shape->getCenter());

        collision_object->setWorldTransform(*(btTransform*)&t);
    }

    virtual void _editorGui() {
        if(ImGui::BeginCombo("Type", "STATIC")) {
            if(ImGui::Selectable("STATIC")) {

            }
            if(ImGui::Selectable("KINEMATIC")) {

            }
            ImGui::EndCombo();
        }
        if(ImGui::CheckboxFlags("Static", &collision_group, 1) ||
            ImGui::CheckboxFlags("Probe", &collision_group, 2) ||
            ImGui::CheckboxFlags("Sensor", &collision_group, 4) ||
            ImGui::CheckboxFlags("Hitbox", &collision_group, 8) ||
            ImGui::CheckboxFlags("Hurtbox", &collision_group, 16)
        ) {
            collision_object->setCollisionFlags(collision_group);
        }

        if (ImGui::BeginCombo("Shape", collision_shape->getType().get_name().to_string().c_str(), 0)) {
            if(ImGui::Selectable(rttr::type::get<CollisionBox>().get_name().to_string().c_str())) {
                setShape<CollisionBox>();
            }
            if(ImGui::Selectable(rttr::type::get<CollisionCapsule>().get_name().to_string().c_str())) {
                setShape<CollisionCapsule>();
            }
            if(ImGui::Selectable(rttr::type::get<CollisionMesh>().get_name().to_string().c_str())) {
                setShape<CollisionMesh>();
            }
            ImGui::EndCombo();
        }

        if(ImGui::Button("Update transform")) {
            updateTransform();
        }

        if(collision_shape->_editorGui(this)) {
            updateTransform();
        }
    }

    virtual void serialize(std::ostream& out) {
        write(out, (int32_t)collision_shape->getShapeType());
        write(out, (uint64_t)collision_group);
        collision_shape->serialize(out);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        CollisionShape::TYPE t = (CollisionShape::TYPE)read<int32_t>(in);
        collision_group = (uint32_t)read<uint64_t>(in);
        switch(t) {
        case CollisionShape::CAPSULE:
            setShape<CollisionCapsule>();
            collision_shape->deserialize(in, sz - sizeof(int32_t));
            collision_object->setCollisionShape(collision_shape->getBtShape());
            updateTransform();
            break;
        case CollisionShape::MESH:
            setShape<CollisionMesh>();
            collision_shape->deserialize(in, sz - sizeof(int32_t));
            collision_object->setCollisionShape(collision_shape->getBtShape());
            updateTransform();
            break;
        default:
            in.seekg(sz - sizeof(int32_t), std::ios_base::cur);
            break;
        }
        
        collision_object->setCollisionFlags(collision_group);
    }
private:
    void resetCollisionObject() {
        world->getBtWorld()->removeCollisionObject(collision_object.get());
        world->getBtWorld()->addCollisionObject(collision_object.get());

        world->getBtWorld()->updateAabbs();
    }

    unsigned collision_group = 1;

    std::shared_ptr<btCollisionObject> collision_object;

    std::shared_ptr<CollisionShape> collision_shape;

    Transform* transform = 0;
    PhysicsWorld* world = 0;
    PhysicsSystem* sys = 0;
};
STATIC_RUN(Collider)
{
    rttr::registration::class_<Collider>("Collider")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
