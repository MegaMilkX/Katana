#ifndef ECS_TRANSFORM_TREE_HPP
#define ECS_TRANSFORM_TREE_HPP


#include "../attribute.hpp"
#include "../../gfxm.hpp"

#include "../world.hpp"

class TransformTreeNode {
    std::string             _name;
    bool                    _dirty = false;
    gfxm::vec3              _position = gfxm::vec3(.0f, .0f, .0f);
    gfxm::quat              _rotation = gfxm::quat(.0f, .0f, .0f, 1.0f);
    gfxm::vec3              _scale = gfxm::vec3(1.0f, 1.0f, 1.0f);
    gfxm::mat4              _world_transform = gfxm::mat4(1.0f);
    TransformTreeNode*           _parent = 0;
    std::set<TransformTreeNode*> _children;

public:
    void setName(const char* name);
    const std::string& getName() const;

    TransformTreeNode* createChild();
    size_t             childCount();
    TransformTreeNode* getChild(size_t id);

    void dirty();
    bool isDirty();

    void lookAt(const gfxm::vec3& tgt, const gfxm::vec3& up, float f = 1.0f);

    void translate(float x, float y, float z);
    void translate(const gfxm::vec3& vec);
    void rotate(float angle, float axisX, float axisY, float axisZ);
    void rotate(float angle, const gfxm::vec3& axis);
    void rotate(const gfxm::quat& q);

    void setPosition(float x, float y, float z);
    void setPosition(const gfxm::vec3& position);
    void setRotation(float x, float y, float z);
    void setRotation(gfxm::vec3 euler);
    void setRotation(const gfxm::quat& rotation);
    void setRotation(float x, float y, float z, float w);
    void setScale(float s);
    void setScale(float x, float y, float z);
    void setScale(const gfxm::vec3& s);
    void setTransform(gfxm::mat4 t);

    const gfxm::vec3& getPosition() const;
    const gfxm::quat& getRotation() const;
    const gfxm::vec3& getScale() const;
    gfxm::vec3        getEulerAngles();
    gfxm::vec3        getWorldPosition();
    gfxm::quat        getWorldRotation();
    gfxm::vec3        getWorldScale();

    gfxm::vec3 right();
    gfxm::vec3 up();
    gfxm::vec3 back();
    gfxm::vec3 left();
    gfxm::vec3 down();
    gfxm::vec3 forward();

    gfxm::mat4        getLocalTransform() const;
    gfxm::mat4        getParentTransform();
    const gfxm::mat4& getWorldTransform();

    void setParent(TransformTreeNode* p);
    TransformTreeNode* getParent() const;

};


class ecsTransformTree : public ecsAttrib<ecsTransformTree> {
    void imguiNode(TransformTreeNode* node) {
        if(node->childCount()) {
            bool open = ImGui::TreeNode(node->getName().c_str());
            if(open) {
                for(size_t i = 0; i < node->childCount(); ++i) {
                    imguiNode(node->getChild(i));
                }

                ImGui::TreePop();
            }
        } else {
            ImGui::Selectable(node->getName().c_str());
        }
    }
public:
    TransformTreeNode root_node;

    TransformTreeNode* getRoot() {
        return &root_node;
    }

    void onGui(ecsWorld* world, entity_id ent) {
        ImGui::PushItemWidth(-1);
        if(ImGui::ListBoxHeader("###OBJECT_LIST", ImVec2(0, 300))) {
            imguiNode(&root_node);
                
            ImGui::ListBoxFooter();
        }
        ImGui::PopItemWidth();
    }
};


#endif
