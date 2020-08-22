#ifndef SUBSCENE_DYNAMICS_HPP
#define SUBSCENE_DYNAMICS_HPP

#include "../../system.hpp"
#include "../../attribs/base_attribs.hpp"

#include "../../util/bullet_debug_draw.hpp"

class ecsSubDynamicsSys;
class ecsTupleCollisionSubScene : public ecsTuple<ecsWorldTransform, ecsSubScene> {
    btDiscreteDynamicsWorld* world = 0;

public:
    ecsSubDynamicsSys* sub_dynamics = 0;

    void reinit(btDiscreteDynamicsWorld* world);

    void onAttribUpdate(ecsSubScene* scn) {
        reinit(world);
    }

};
class ecsTupleCollisionCache : public ecsTuple<ecsCollisionCache> {
public:
};
class ecsArchCollider : public ecsTuple<
    ecsWorldTransform, 
    ecsCollisionShape, 
    ecsExclude<ecsMass>, 
    ecsOptional<ecsCollisionCache>,
    ecsOptional<ecsCollisionFilter>
> {
public:
    void onAttribUpdate(ecsCollisionShape* shape) override {
        if(collision_object && get_optional<ecsCollisionCache>()) {
            collision_object->setUserPointer(get_optional<ecsCollisionCache>());
        }
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(shape->shape.get());

        ecsCollisionFilter* filter = get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        //LOG("Collider shape changed");
    }

    void onAddOptional(ecsCollisionCache* cache) override {
        if(collision_object) {
            collision_object->setUserPointer((void*)cache);
        }
    }
    void onRemoveOptional(ecsCollisionCache* cache) override {
        if(collision_object) {
            collision_object->setUserPointer(0);
        }
    }

    void onAddOptional(ecsCollisionFilter* filter) override {
        if(collision_object && world) {
            world->removeCollisionObject(collision_object.get());
            uint64_t group = filter->group;
            uint64_t mask = filter->mask;
            world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        }
    }
    void onRemoveOptional(ecsCollisionFilter* filter) override {
        if(collision_object && world) {
            world->removeCollisionObject(collision_object.get());
            uint64_t group = 1;
            uint64_t mask = 1;
            world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        }
    }

    btCollisionWorld* world = 0;
    std::shared_ptr<btCollisionObject> collision_object;
};
class ecsTupleCollisionPlane : public ecsTuple<
    ecsCollisionPlane,
    ecsOptional<ecsCollisionCache>,
    ecsOptional<ecsCollisionFilter>
> {
public:
    void onAttribUpdate(ecsCollisionPlane* p) override {
        if(collision_object && get_optional<ecsCollisionCache>()) {
            collision_object->setUserPointer(get_optional<ecsCollisionCache>());
        }
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(p->shape.get());

        ecsCollisionFilter* filter = get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
    }

    void onAddOptional(ecsCollisionCache* cache) override {
        if(collision_object) {
            collision_object->setUserPointer((void*)cache);
        }
    }
    void onRemoveOptional(ecsCollisionCache* cache) override {
        if(collision_object) {
            collision_object->setUserPointer(0);
        }
    }

    void onAddOptional(ecsCollisionFilter* filter) override {
        if(collision_object && world) {
            world->removeCollisionObject(collision_object.get());
            uint64_t group = filter->group;
            uint64_t mask = filter->mask;
            world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        }
    }
    void onRemoveOptional(ecsCollisionFilter* filter) override {
        if(collision_object && world) {
            world->removeCollisionObject(collision_object.get());
            uint64_t group = 1;
            uint64_t mask = 1;
            world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        }
    }

    btCollisionWorld* world = 0;
    std::shared_ptr<btCollisionObject> collision_object;
};
class ecsTplCollisionMesh : public ecsTuple<
    ecsWorldTransform, ecsCollisionMesh,
    ecsOptional<ecsCollisionCache>, ecsOptional<ecsCollisionFilter>
> {
public:
    btCollisionWorld*                   world = 0;
    std::unique_ptr<btCollisionObject>  collision_object;

    void onAttribUpdate(ecsCollisionMesh* p) override {
        if(collision_object && get_optional<ecsCollisionCache>()) {
            collision_object->setUserPointer(get_optional<ecsCollisionCache>());
        }
        world->removeCollisionObject(collision_object.get());
        collision_object->setCollisionShape(p->shape.get());

        ecsCollisionFilter* filter = get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
    }

    void onAddOptional(ecsCollisionCache* cache) override {
        if(collision_object) {
            collision_object->setUserPointer((void*)cache);
        }
    }
    void onRemoveOptional(ecsCollisionCache* cache) override {
        if(collision_object) {
            collision_object->setUserPointer(0);
        }
    }

    void onAddOptional(ecsCollisionFilter* filter) override {
        if(collision_object && world) {
            world->removeCollisionObject(collision_object.get());
            uint64_t group = filter->group;
            uint64_t mask = filter->mask;
            world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        }
    }
    void onRemoveOptional(ecsCollisionFilter* filter) override {
        if(collision_object && world) {
            world->removeCollisionObject(collision_object.get());
            uint64_t group = 1;
            uint64_t mask = 1;
            world->addCollisionObject(collision_object.get(), (int)group, (int)mask);
        }
    }
};
class ecsArchRigidBody : public ecsTuple<
    ecsWorldTransform, 
    ecsCollisionShape, 
    ecsMass,
    ecsOptional<ecsCollisionCache>,
    ecsOptional<ecsCollisionFilter>
> {
public:
    void onAttribUpdate(ecsWorldTransform* t) override {
        //auto& transform = rigid_body->getWorldTransform();        
        btTransform transform;
        const gfxm::mat4& world_transform = t->getTransform();
        transform.setFromOpenGLMatrix((float*)&world_transform);
        
        //LOG(world_transform);
        rigid_body->setWorldTransform(transform);
        world->updateSingleAabb(rigid_body.get());
    }

    void onAttribUpdate(ecsMass* m) override {
        rigid_body->setMassProps(m->mass, btVector3(.0f, .0f, .0f));
    }

    void onAttribUpdate(ecsCollisionShape* shape) override {
        if(rigid_body && get_optional<ecsCollisionCache>()) {
            rigid_body->setUserPointer(get_optional<ecsCollisionCache>());
        }
        world->removeRigidBody(rigid_body.get());
        rigid_body->setCollisionShape(shape->shape.get());

        ecsCollisionFilter* filter = get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addRigidBody(rigid_body.get(), (int)group, (int)mask);
        //LOG("RigidBody shape changed");
    }

    void onAddOptional(ecsCollisionCache* cache) override {
        if(rigid_body) {
            rigid_body->setUserPointer((void*)cache);
        }
    }
    void onRemoveOptional(ecsCollisionCache* cache) override {
        if(rigid_body) {
            rigid_body->setUserPointer(0);
        }
    }

    void onAddOptional(ecsCollisionFilter* filter) override {
        if(rigid_body && world) {
            world->removeRigidBody(rigid_body.get());
            uint64_t group = filter->group;
            uint64_t mask = filter->mask;
            world->addRigidBody(rigid_body.get(), (int)group, (int)mask);
        }
    }
    void onRemoveOptional(ecsCollisionFilter* filter) override {
        if(rigid_body && world) {
            world->removeRigidBody(rigid_body.get());
            uint64_t group = 1;
            uint64_t mask = 1;
            world->addRigidBody(rigid_body.get(), (int)group, (int)mask);
        }
    }

    btDiscreteDynamicsWorld* world = 0;
    std::shared_ptr<btRigidBody> rigid_body;
    btDefaultMotionState motion_state;
};
class ecsTupleKinematicCharacter : public ecsTuple<
    ecsKinematicCharacter,
    ecsCollisionShape
> {};


class ecsSubDynamicsSys : public ecsSystem<
    ecsArchCollider,
    ecsTplCollisionMesh,
    ecsArchRigidBody,
    ecsTupleCollisionCache
> {
public:
    gfxm::mat4 root_transform = gfxm::mat4(1.0f);
    btDiscreteDynamicsWorld* world = 0;

    ecsSubDynamicsSys(btDiscreteDynamicsWorld* world)
    : world(world) {}

    void updateRootTransform(const gfxm::mat4& t) {
        root_transform = t;
    }

    void updateColliders() {
        assert(world);
        for(auto& a : get_array<ecsArchCollider>()) {
            auto& matrix = a->get<ecsWorldTransform>()->getTransform();
            btTransform btt;
            btt.setFromOpenGLMatrix((float*)&matrix);
            a->collision_object->setWorldTransform(btt);
            world->updateSingleAabb(a->collision_object.get());
        }

        for(auto& a : get_array<ecsTplCollisionMesh>()) {
            auto& matrix = a->get<ecsWorldTransform>()->getTransform();
            btTransform btt;
            btt.setFromOpenGLMatrix((float*)&matrix);
            a->collision_object->setWorldTransform(btt);
            world->updateSingleAabb(a->collision_object.get());
        }

        for(auto& a : get_array<ecsTupleCollisionCache>()) {
            a->get<ecsCollisionCache>()->entities.clear();
        }

        // TODO: Rigid bodies?
    }

    void onFit(ecsArchCollider* col) override {
        assert(world);
        col->world = world;
        col->collision_object.reset(new btCollisionObject());
        col->collision_object->setUserIndex((int)col->getEntityUid()); // TODO: Bad, entity_id is 64 bit FIX THIS
        col->collision_object->setUserIndex2(getWorld()->getWorldIndex());
        col->collision_object->setCollisionShape(
            col->get<ecsCollisionShape>()->shape.get()
        );
        if(col->get_optional<ecsCollisionCache>()) {
            col->collision_object->setUserPointer(col->get_optional<ecsCollisionCache>());
        }

        btTransform btt;
        gfxm::mat4 m = root_transform * col->get<ecsWorldTransform>()->getTransform();
        btt.setFromOpenGLMatrix((float*)&m);
        col->collision_object->setWorldTransform(btt);

        ecsCollisionFilter* filter = col->get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(col->collision_object.get(), (int)group, (int)mask);
    }
    void onUnfit(ecsArchCollider* col) override {
        assert(world);
        world->removeCollisionObject(
            col->collision_object.get()
        );
    }

    void onFit(ecsTplCollisionMesh* col) override {
        assert(world);
        col->world = world;
        auto& c = col->collision_object;
        c.reset(new btCollisionObject());
        c->setUserIndex((int)col->getEntityUid());
        c->setUserIndex2(getWorld()->getWorldIndex());
        c->setCollisionShape(col->get<ecsCollisionMesh>()->shape.get());
        if(col->get_optional<ecsCollisionCache>()) {
            c->setUserPointer(col->get_optional<ecsCollisionCache>());
        }

        btTransform btt;
        gfxm::mat4 m = col->get<ecsWorldTransform>()->getTransform();
        btt.setFromOpenGLMatrix((float*)&m);
        c->setWorldTransform(btt);

        ecsCollisionFilter* filter = col->get_optional<ecsCollisionFilter>();
        uint64_t group = 1;
        uint64_t mask = 1;
        if(filter) {
            group = filter->group;
            mask = filter->mask;
        }

        world->addCollisionObject(c.get(), (int)group, (int)mask);
    }
    void onUnfit(ecsTplCollisionMesh* col) override {
        assert(world);
        world->removeCollisionObject(
            col->collision_object.get()
        );
    }
};


inline void ecsTupleCollisionSubScene::reinit(btDiscreteDynamicsWorld* world) {
    this->world = world;
    sub_dynamics = get<ecsSubScene>()->getWorld()->getSystem<ecsSubDynamicsSys>(world);
    sub_dynamics->world = world;
    sub_dynamics->updateRootTransform(
        get<ecsWorldTransform>()->getTransform()
    );
}


#endif
