#ifndef DYNAMICS_CTRL_HPP
#define DYNAMICS_CTRL_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"

#include "../../../common/util/bullet_debug_draw.hpp"

#include "../../attributes/rigid_body.hpp"
#include "../../attributes/collider.hpp"
#include "../../attributes/collision_listener.hpp"

#include <algorithm>

enum COLLISION_GROUP {
    COLLISION_GROUP_0,
    COLLISION_GROUP_1,
    COLLISION_GROUP_2,
    COLLISION_GROUP_3,
    COLLISION_GROUP_4,
    COLLISION_GROUP_5
};

class DynamicsCtrl : public SceneEventFilter<Collider, RigidBody, CollisionListener> {
    RTTR_ENABLE(SceneController)
public:
    struct ColliderInfo {
        std::shared_ptr<btCollisionObject> bt_object;
        int transform_sync_id = -1;
    };
    struct RigidBodyInfo {
        std::shared_ptr<btRigidBody> bt_object;
        btDefaultMotionState motion_state;
        int transform_sync_id = -1;
    };
    std::map<Collider*, ColliderInfo> colliders_;
    std::map<RigidBody*, RigidBodyInfo> rigid_bodies;
    std::map<ktNode*, CollisionListener*> col_listeners;

    void onAttribCreated(Collider* c) override {
        auto& col_info = colliders_[c];
        col_info.bt_object.reset(new btCollisionObject());
        col_info.bt_object->setCollisionShape(c->getShape()->getBtShape());
        col_info.bt_object->setUserPointer((void*)c->getOwner());
        
        if(c->isGhost()) {
            col_info.bt_object->setCollisionFlags(
                col_info.bt_object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE
            );
        }
        col_info.bt_object->setCollisionFlags(
            col_info.bt_object->getCollisionFlags() | btCollisionObject::CF_HAS_CUSTOM_DEBUG_RENDERING_COLOR
        );
        auto& col = c->getDebugColor();
        col_info.bt_object->setCustomDebugColor(btVector3(col.x, col.y, col.z));

        getBtWorld()->addCollisionObject(
            col_info.bt_object.get(),
            c->getCollisionGroup(),
            c->getCollisionMask()
        );
    }
    void onAttribRemoved(Collider* c) override {
        auto it = colliders_.find(c);
        if(it == colliders_.end()) return;
        auto& col_info = it->second;
        getBtWorld()->removeCollisionObject(col_info.bt_object.get());
        colliders_.erase(c);
    }
    void onAttribCreated(RigidBody* rb) override {
        auto& rb_info = rigid_bodies[rb];
        auto bt_shape = rb->getShape()->getBtShape();
        btVector3 local_inertia;
        bt_shape->calculateLocalInertia(rb->getMass(), local_inertia);
        btTransform btt;
        btTransform com;
        btt.setFromOpenGLMatrix(
            (float*)&rb->getOwner()->getTransform()->getWorldTransform()
        );
        com.setFromOpenGLMatrix(
            (float*)&gfxm::mat4(1.0f)
        );
        rb_info.motion_state = btDefaultMotionState(
            btt, com
        );
        rb_info.bt_object.reset(new btRigidBody(
            rb->getMass(),
            &rb_info.motion_state,
            bt_shape,
            local_inertia
        ));
        rb_info.bt_object->setUserPointer((void*)rb->getOwner());

        getBtWorld()->addRigidBody(
            rb_info.bt_object.get(), 
            rb->getCollisionGroup(), 
            rb->getCollisionMask()
        );
    }
    void onAttribRemoved(RigidBody* rb) override {
        auto it = rigid_bodies.find(rb);
        if(it == rigid_bodies.end()) return;
        auto& rb_info = it->second;
        getBtWorld()->removeRigidBody(rb_info.bt_object.get());
        rigid_bodies.erase(rb);
    }
    void onAttribCreated(CollisionListener* l) override {
        col_listeners[l->getOwner()] = l;
    }
    void onAttribRemoved(CollisionListener* l) override {
        col_listeners.erase(l->getOwner());
    }

    DynamicsCtrl();
    ~DynamicsCtrl();

    const std::string& getCollisionGroupName(COLLISION_GROUP grp) {
        return col_group_names[grp];
    }

    bool getAdjustedPosition(btCollisionObject* o, gfxm::vec3& pos, uint32_t mask = 1);
    bool getAdjustedPosition(Collider* c, gfxm::vec3& pos, uint32_t mask = 1);

    bool sweepSphere(
        float radius, 
        const gfxm::vec3& from, 
        const gfxm::vec3& to, 
        gfxm::vec3& hit,
        uint32_t mask = 1);

    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ true, FRAME_PRIORITY_DYNAMICS };
    }
    virtual void onStart() {
        update(1.0f/60.0f);
    }
    virtual void onUpdate() {
        update(1.0f/60.0f);
    }
    virtual void debugDraw(DebugDraw& dd);
    virtual void setDebugDraw(DebugDraw* dd) {
        debugDrawer.setDD(dd);
    }

    btDynamicsWorld* getBtWorld();

    void update(float dt);

    // For use in editor
    void updateBodyTransforms();

    virtual void onGui() {
        ImGui::Text("Collision groups");
        char buf[256];
        memcpy(buf, col_group_names[0].data(), col_group_names[0].size());
        ImGui::InputText("0", buf, 256);
        memcpy(buf, col_group_names[1].data(), col_group_names[1].size());
        ImGui::InputText("1", buf, 256);
        memcpy(buf, col_group_names[2].data(), col_group_names[2].size());
        ImGui::InputText("2", buf, 256);
        memcpy(buf, col_group_names[3].data(), col_group_names[3].size());
        ImGui::InputText("3", buf, 256);
        memcpy(buf, col_group_names[4].data(), col_group_names[4].size());
        ImGui::InputText("4", buf, 256);
        memcpy(buf, col_group_names[5].data(), col_group_names[5].size());
        ImGui::InputText("5", buf, 256);
    }
private:
    std::string col_group_names[6];

    std::set<std::pair<ktNode*, ktNode*>> pair_cache;

    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
	//btCollisionWorld* world;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

    BulletDebugDrawer2_OpenGL debugDrawer;
};
STATIC_RUN(DynamicsCtrl) {
    rttr::registration::class_<DynamicsCtrl>("DynamicsCtrl")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
