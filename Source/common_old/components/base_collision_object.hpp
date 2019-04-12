#ifndef BASE_COLLISION_OBJECT_HPP
#define BASE_COLLISION_OBJECT_HPP

#include "collision_shapes.hpp"
#include "../component.hpp"
#include "../transform.hpp"
#include "../gfxm.hpp"
#include "../scene_components/scene_physics_world.hpp"

template<typename BT_COL_OBJECT_T, typename COL_SHAPE_T>
class CollisionObject : public Component {
    CLONEABLE
    RTTR_ENABLE(Component)
public:
    CollisionObject() {
        bt_object.reset(new BT_COL_OBJECT_T());
        bt_object->setCollisionShape(shape.getBtShape());
    }
    virtual ~CollisionObject() {
        world->getBtWorld()->removeCollisionObject(bt_object.get());
    }
    virtual void onCreate() {
        bt_object->setUserPointer(getObject());

        world = getScene()->getSceneComponent<PhysicsWorld>();
        world->getBtWorld()->addCollisionObject(bt_object.get());
        transform = getObject()->get<Transform>();

        updateTransform();
    }
    void onClone(CollisionObject* other) {
        center = other->center;
        shape = other->shape;
        updateTransform();
    }

    bool contactTest(gfxm::vec3& new_pos, unsigned flags) {
        return world->contactTest(bt_object.get(), new_pos, flags);
    }

    void updateTransform() {
        gfxm::mat4 t = 
            gfxm::translate(gfxm::mat4(1.0f), transform->worldPosition() + center);
        bt_object->setWorldTransform(*(btTransform*)&t);
    }

    virtual void _editorGui() {
        if(ImGui::DragFloat3("Center", (float*)&center, 0.01f)) {}
        shape._editorGui();

        updateTransform();
    }

    virtual void serialize(std::ostream& out) {
        write(out, group_mask);
        write(out, center);
        shape.serialize(out);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        group_mask = read<uint64_t>(in);
        center = read<gfxm::vec3>(in);
        shape.deserialize(in, sz - sizeof(gfxm::vec3) - sizeof(uint64_t));

        updateTransform();
    }
protected:
    uint64_t group_mask = 0;
    gfxm::vec3 center;
    std::shared_ptr<BT_COL_OBJECT_T> bt_object;
    COL_SHAPE_T shape;
    PhysicsWorld* world = 0;
    Transform* transform = 0;
};

#endif
