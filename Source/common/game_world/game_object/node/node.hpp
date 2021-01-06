#ifndef KT_ACTOR_NODE_HPP
#define KT_ACTOR_NODE_HPP

#include <string>
#include "../../../gfxm.hpp"

#include "../../../lib/imgui_wrap.hpp"
#include "../../../util/imgui_helpers.hpp"

#include "../entity.hpp"

class ktActor;
class ktActorNode : public ktEntity {
protected:
    std::string name;

    ktActor* actor;

    gfxm::mat4 local;
    gfxm::mat4 world;

public:
    ktActorNode* parent = 0;
    ktActorNode* next_sibling = 0;
    ktActorNode* first_child = 0;

    gfxm::vec3 translation;
    gfxm::quat rotation = gfxm::quat(0,0,0,1);
    gfxm::vec3 scale = gfxm::vec3(1,1,1);

    ktActorNode(ktActor* owner);
    ~ktActorNode();

    ktActor* getActor() { return actor; }

    const std::string& getName() const { return name; }
    void               setName(const std::string& name) { this->name = name; }

    const gfxm::mat4& getLocalTransform() {
        return local = gfxm::translate(gfxm::mat4(1.0f), translation) *
            gfxm::to_mat4(rotation) *
            gfxm::scale(gfxm::mat4(1.0f), scale);
    }
    const gfxm::mat4& getWorldTransform() {
        if(parent != 0) {
            return world = parent->getWorldTransform() * getLocalTransform();
        } else {
            return world = getLocalTransform();
        }
    }

    virtual void onGui() {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, name.data(), name.size());
        if(ImGui::InputText("name", buf, 256)) {
            setName(buf);
        }
        ImGui::DragFloat3("translation", (float*)&translation, 0.01f);
        ImGui::DragFloat4("rotation", (float*)&rotation, 0.01f);
        ImGui::DragFloat3("scale", (float*)&scale, 0.01f);
    }
};

struct ktRenderData {
    gfxm::aabb   aabb;
    gfxm::aabb   aabb_transformed;
    DrawCmdSolid cmd;
    int32_t      octree_object;
    bool         visible;
};
class ktNodeRenderable : public ktActorNode {
protected:
    ktRenderData render_data;
public:
    ktNodeRenderable(ktActor* owner);
    ~ktNodeRenderable();

    ktRenderData* getRenderData() { return &render_data; }
};
#include "../../../resource/mesh.hpp"
class ktNodeMesh : public ktNodeRenderable {
    std::shared_ptr<Mesh> mesh;
public:
    ktNodeMesh(ktActor* owner);
    ~ktNodeMesh();

    void setMesh(const std::shared_ptr<Mesh>& mesh);

    void onGui() override;
};
class ktNodeLightOmni : public ktActorNode {
public:
    ktNodeLightOmni(ktActor* owner) : ktActorNode(owner) {}
    gfxm::vec3 color = gfxm::vec3(1,1,1);
    float radius = 3.0f;
    float intensity = 10.0f;

    void onGui() override {
        ktActorNode::onGui();
    }
};

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
class ktNodeRigidBody : public ktActorNode {
    std::unique_ptr<btRigidBody> body;
    std::unique_ptr<btCollisionShape> shape;
    btDefaultMotionState motion_state;
    float mass = 1.0f;
public:
    ktNodeRigidBody(ktActor* owner);
    ~ktNodeRigidBody();
};


#endif
