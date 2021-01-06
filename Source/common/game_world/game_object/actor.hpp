#ifndef KT_ACTOR_HPP
#define KT_ACTOR_HPP

#include "game_object.hpp"
#include "node/node.hpp"

class ktActor : public ktGameObject {
    std::unique_ptr<ktActorNode> root;

public:

    ktCoolComponent cool;
    ktVeryCoolComponent very_cool;

    ktActor(ktGameWorld* world)
    : ktGameObject(world) {
        root.reset(new ktActorNode(this));
        root->setName("Root");

        createComponent<ktTransformComponent>();
        createComponent<ktMeshComponent>();

        createComponent<ktCTranslation>();
        createComponent<ktCRotation>();
        createComponent<ktCScale>();
        createComponent<ktCWorldTransform>();

        enableComponent(&cool);
        enableComponent(&very_cool);
    }

    ktActorNode* getRoot() { return root.get(); }
    void         addChild(ktActorNode* parent, ktActorNode* child) {
        if(!parent->first_child) {
            parent->first_child = child;
        } else {
            ktActorNode* node = parent->first_child;
            while(node->next_sibling) {
                node = node->next_sibling;
            }
            node->next_sibling = child;
        }

        child->parent = parent;
    }

    void onGui() override {
        ImGui::Text("Hello ktActor!");
    }
};

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include "../../resource/motion.hpp"
#include "../render_scene.hpp"
class ktCharacter : public ktGameObject {
    std::shared_ptr<Model_> model;
    std::unique_ptr<btCollisionObject> collision_hull;
    std::unique_ptr<btCollisionShape> hull_shape;
    std::shared_ptr<Motion> motion;

public:
    ktCharacter(ktGameWorld* world);
    ~ktCharacter();

    void onGui() override {
        ImGui::Text("Hello ktCharacter!");
    }
};


#endif
