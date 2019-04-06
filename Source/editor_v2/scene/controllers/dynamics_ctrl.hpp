#ifndef DYNAMICS_CTRL_HPP
#define DYNAMICS_CTRL_HPP

#include "../scene_controller.hpp"
#include "../game_scene.hpp"

#include "../../../common/util/bullet_debug_draw.hpp"

class CmCollisionShape;
class CmCollisionObject;
class CmRigidBody;
class GhostObject;

class DynamicsCtrl : public SceneController {
    RTTR_ENABLE(SceneController)
public:
    DynamicsCtrl();
    ~DynamicsCtrl();

    bool getAdjustedPosition(btCollisionObject* o, gfxm::vec3& pos);

    void sweepTest(GhostObject* go, const gfxm::mat4& from, const gfxm::mat4& to);
    bool sweepSphere(float radius, const gfxm::vec3& from, const gfxm::vec3& to, gfxm::vec3& hit);

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

    void _addCollider(CmCollisionObject* col);
    void _removeCollider(CmCollisionObject* col);
    void _addGhost(GhostObject* col);
    void _removeGhost(GhostObject* col);
    void _addRigidBody(CmRigidBody* col);
    void _removeRigidBody(CmRigidBody* col);
    void _addCollisionShape(CmCollisionShape* s);
    void _removeCollisionShape(CmCollisionShape* s);

    void _shapeChanged(CmCollisionShape* s);
private:
    std::set<CmCollisionObject*> colliders;
    std::set<GhostObject*> ghosts;
    std::set<CmRigidBody*> bodies;
    std::set<CmCollisionShape*> shapes;

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
