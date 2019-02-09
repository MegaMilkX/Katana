#ifndef COLLISION_SENSOR_HPP
#define COLLISION_SENSOR_HPP

#include "collision_component_base.hpp"
#include "../transform.hpp"
#include "../scene_components/scene_physics_world.hpp"
#include "../scene_object.hpp"
#include "collision_shape.hpp"
#include "../util/serialization.hpp"
#include <bulletcollision/collisiondispatch/btghostobject.h>

class PhysicsSystem;
class CollisionSensor : public BaseCollisionComponent {
    CLONEABLE
    RTTR_ENABLE(BaseCollisionComponent)
public:
    CollisionSensor() {
        ghost_object.reset(new btGhostObject());
        ghost_object->setUserPointer(this);
        ghost_object->setCollisionFlags(
            ghost_object->getCollisionFlags() | 
            btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        ghost_object->setCustomDebugColor(btVector3(1.0f, .15f, 0.25f));
    }
    ~CollisionSensor() {
        world->getBtWorld()->removeCollisionObject(ghost_object.get());
    }

    void onClone(CollisionSensor* other) {
        shape.reset(other->shape->clone());
        ghost_object->setCollisionShape(shape->getBtShape());
        group_mask = other->group_mask;

        updateTransform();
        resetBtObject();
    }

    virtual void onCreate();

    bool contactTest(gfxm::vec3& new_pos, unsigned flags) {
        return world->contactTest(ghost_object.get(), new_pos, flags);
    }

    template<typename T>
    void setShape() {
        shape.reset(new T());
        refreshShape();
    }

    void refreshShape() {
        ghost_object->setCollisionShape(shape->getBtShape());
    }

    void updateTransform() {
        gfxm::mat4 t = 
            gfxm::translate(gfxm::mat4(1.0f), transform->worldPosition() + shape->getCenter());
        ghost_object->setWorldTransform(*(btTransform*)&t);
    }

    virtual void serialize(std::ostream& out) {}
    virtual void deserialize(std::istream& in, size_t sz) {}
    virtual void _editorGui() {
        bool group_changed = false;
        group_changed = ImGui::CheckboxFlags("Static", &group_mask, 1); ImGui::SameLine();
        group_changed = ImGui::CheckboxFlags("Probe", &group_mask, 2);
        group_changed = ImGui::CheckboxFlags("Sensor", &group_mask, 4); ImGui::SameLine();
        group_changed = ImGui::CheckboxFlags("Hitbox", &group_mask, 8);
        group_changed = ImGui::CheckboxFlags("Hurtbox", &group_mask, 16);
        if(group_changed) {
            ghost_object->setCollisionFlags(group_mask);
        }

        if (ImGui::BeginCombo("Shape", shape->getType().get_name().to_string().c_str(), 0)) {
            if(ImGui::Selectable(rttr::type::get<CollisionBox>().get_name().to_string().c_str())) {
                setShape<CollisionBox>();
            }
            if(ImGui::Selectable(rttr::type::get<CollisionCapsule>().get_name().to_string().c_str())) {
                setShape<CollisionCapsule>();
            }
            ImGui::EndCombo();
        }

        if(ImGui::Button("Update transform")) {
            updateTransform();
        }

        if(shape->_editorGui(this)) {
            updateTransform();
        }
    }
private:
    void resetBtObject() {
        world->getBtWorld()->removeCollisionObject(ghost_object.get());
        world->getBtWorld()->addCollisionObject(ghost_object.get());

        world->getBtWorld()->updateAabbs();
    }

    std::shared_ptr<btGhostObject> ghost_object;
    std::shared_ptr<CollisionShape> shape;

    Transform* transform = 0;
    PhysicsWorld* world = 0;
    unsigned group_mask = 2;
};
STATIC_RUN(CollisionSensor)
{
    rttr::registration::class_<CollisionSensor>("CollisionSensor")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif