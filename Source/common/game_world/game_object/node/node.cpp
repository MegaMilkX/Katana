#include "node.hpp"

#include "../actor.hpp"
#include "../../game_world.hpp"

#include "../../../ecs/attribs/base_attribs.hpp"

ktActorNode::ktActorNode(ktActor* owner) 
: ktEntity(owner->getWorld()), actor(owner) {
}
ktActorNode::~ktActorNode() {}


ktNodeRenderable::ktNodeRenderable(ktActor* owner)
: ktActorNode(owner) {
    render_data.cmd.indexCount = 0;
    render_data.cmd.lightmap = 0;
    render_data.aabb = gfxm::aabb(gfxm::vec3(-0.5f, -0.5f, -0.5f), gfxm::vec3(0.5f, 0.5f, 0.5f));
    getActor()->getWorld()->getRenderScene()->addRenderable(this);
}
ktNodeRenderable::~ktNodeRenderable() {
    getActor()->getWorld()->getRenderScene()->removeRenderable(this);
}


ktNodeMesh::ktNodeMesh(ktActor* owner) : ktNodeRenderable(owner) {}
ktNodeMesh::~ktNodeMesh() {}
void ktNodeMesh::setMesh(const std::shared_ptr<Mesh>& mesh) {
    this->mesh = mesh;
    if(mesh) {
        auto& rd = render_data;
        rd.aabb = mesh->aabb;
        rd.cmd.indexCount = mesh->indexCount();
        rd.cmd.indexOffset = 0;
        rd.cmd.lightmap = 0;
        rd.cmd.material = 0;
        rd.cmd.object_ptr = 0;
        rd.cmd.vao = mesh->mesh.getVao();
        rd.cmd.transform = getWorldTransform();
    }
}
void ktNodeMesh::onGui() {
    ktActorNode::onGui();
    imguiResourceTreeCombo("mesh", mesh, "msh", [this](){
        setMesh(mesh);
    });
}


ktNodeRigidBody::ktNodeRigidBody(ktActor* owner)
: ktActorNode(owner) {
    auto btWorld = owner->getWorld()->getDynamicsWorld()->getBtWorld();
    shape.reset(new btSphereShape(0.5f));
    btVector3 local_inertia;
    btTransform btt, center_of_mass;
    shape->calculateLocalInertia(mass, local_inertia);
    btt.setIdentity();
    center_of_mass.setIdentity();
    motion_state = btDefaultMotionState(btt, center_of_mass);
    body.reset(new btRigidBody(mass, &motion_state, shape.get(), local_inertia));

    btWorld->addRigidBody(body.get());
}
ktNodeRigidBody::~ktNodeRigidBody() {
    auto btWorld = getActor()->getWorld()->getDynamicsWorld()->getBtWorld();
    btWorld->removeRigidBody(body.get());
}