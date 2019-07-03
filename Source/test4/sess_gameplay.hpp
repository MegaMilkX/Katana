#ifndef SESS_GAMEPLAY_HPP
#define SESS_GAMEPLAY_HPP

#include "core/session.hpp"
#include "../common/input_listener.hpp"
#include "../common/gfxm.hpp"

#include "../editor/scene_object_from_fbx.hpp"

#include "../common/components/model.hpp"
#include "../common/components/animation_stack.hpp"

#include "../common/scene/controllers/dynamics_ctrl.hpp"
#include "../common/scene/controllers/render_controller.hpp"

#include <iostream>

class actorGameCamera : public Actor, public InputListenerWrap {
    GameObject* node = 0;
public:
    actorGameCamera() {
        
    }

    void onInit() {
        node = getSession()->getScene()->getRoot()->createChild();
        getSession()->getScene()->getController<RenderController>()
            ->setDefaultCamera(node->get<Camera>().get());
        node->get<Camera>()->setZFar(1000.0f);
        node->get<Camera>()->setFov(0.76);

        auto t = node->getTransform();
        t->translate(-1.8, 3, 7);
        t->rotate(-0.2f, gfxm::vec3(1,0,0));
    }
};

class actorFreeCamera : public Actor, public InputListenerWrap {
    float angle_y = 0.0f;
    float angle_x = 0.0f;
    gfxm::vec2 angle_velo;
    gfxm::vec2 velo;

    GameObject* node = 0;
public:
    actorFreeCamera() {
        bindAxis("MoveHori", [this](float v){
            velo.x = v;
        });
        bindAxis("MoveVert", [this](float v){
            velo.y = v;
        });
        bindAxis("MoveCamX", [this](float v){
            angle_velo.y = -v * 0.01f;
        });
        bindAxis(
            "MoveCamY",
            [this](float v) {
                angle_velo.x = -v * 0.01f;
            }
        );
    }

    virtual void onInit() {
        node = getSession()->getScene()->getRoot()->createChild();
        getSession()->getScene()->getController<RenderController>()
            ->setDefaultCamera(node->get<Camera>().get());
        node->get<Camera>()->setZFar(1000.0f);
        node->get<Camera>()->setFov(0.76);
    }

    virtual void onUpdate() {
        auto t = node->getTransform();
        t->translate(t->right() * velo.x * (1/60.0f) * 3);
        t->translate(t->forward() * velo.y * (1/60.0f) * 3);

        t->rotate(angle_velo.x, t->getWorldTransform() * gfxm::vec4(1,0,0,0) );
        t->rotate(angle_velo.y, gfxm::vec3(0,1,0));
    }
};

class actorTpsCamera : public Actor, public InputListenerWrap {
    gfxm::vec3 pivot = gfxm::vec3(0.0f, 1.3f, 0.0f);
    float distance = 10.0f;
    float angle_y = 0.0f;
    float angle_x = 0.0f;
    float _distance = 10.0f;
    float _angle_y = 0.0f;
    float _angle_x = 0.0f;

    GameObject* node = 0;
public:
    actorTpsCamera() {
        bindAxis("MoveCamX", [this](float v){
            angle_y -= v * 0.01f;
        });
        bindAxis(
            "MoveCamY",
            [this](float v) {
                angle_x -= v * 0.01f;
                if(angle_x > gfxm::pi * 0.10f) {
                    angle_x = gfxm::pi * 0.10f;
                } else if(angle_x < -gfxm::pi * 0.45f) {
                    angle_x = -gfxm::pi * 0.45f;
                }
            }
        );
        bindAxis(
            "CameraZoom",
            [this](float v) {
                float mod = distance;
                distance += -v * mod * 0.15f;
                if(distance < 1.0f) {
                    distance = 1.0f;
                } else if(distance > 3.0f) {
                    distance = 3.0f;
                }
            }
        );
    }

    virtual void onInit() {
        node = getSession()->getScene()->getRoot()->createChild();
        getSession()->getScene()->getController<RenderController>()
            ->setDefaultCamera(node->get<Camera>().get());
        node->get<Camera>()->setZFar(1000.0f);
        node->get<Camera>()->setFov(0.76);
    }

    virtual void onUpdate() {
        gfxm::vec3 tgt_pos;
        TransformNode* t = 0;
        float height = 1.6f;
        //if(tgt_chara) {
        //    t = tgt_chara->getTransform();
        //    height = tgt_chara->getCameraHeight();
        //}

        if(t) {
            tgt_pos = t->getWorldPosition() + gfxm::vec3(0.0f, height, 0.0f);
        } else {
            tgt_pos = gfxm::vec3(0.0,height,0.0);
        }

        pivot = gfxm::lerp(pivot, tgt_pos, std::pow(gfxm::clamp(gfxm::length(tgt_pos - pivot), 0.0f, 1.0f), 2.0f));

        _distance = gfxm::lerp(_distance, distance, 0.1f);
        _angle_y = gfxm::lerp(_angle_y, angle_y, 0.1f);
        _angle_x = gfxm::lerp(_angle_x, angle_x, 0.1f);
        gfxm::transform tcam;
        tcam.position(pivot);
        tcam.rotate(_angle_y, gfxm::vec3(0.0f, 1.0f, 0.0f));
        tcam.rotate(_angle_x, tcam.right());

        gfxm::vec3 hit;
        float sweepRad = 0.3f;
        if(getSession()->getScene()->getController<DynamicsCtrl>()
            ->sweepSphere(
                sweepRad,
                pivot,
                pivot + tcam.back() * _distance,
                hit
            )
        ) {
            float len = gfxm::length(pivot - hit);
            _distance = len;
        }

        tcam.translate(tcam.back() * _distance);
        node->getTransform()->setTransform(tcam.matrix());
    }
};

class actorPlayerShip : public Actor, public InputListenerWrap {
    int wpn_type = 0;
    int wpn_lv = 1;
    int speed_lv = 1;
    int missile_lv = 0;
    int option_lv = 0;
    int shield_state = 0;

    gfxm::vec2 velo_dir;

    GameObject* node = 0;
public:
    actorPlayerShip() {
        bindAxis("MoveHori", [this](float v){
            //velo_dir.x = v;
        });
        bindAxis("MoveVert", [this](float v){
            //velo_dir.y = v;
        });
    }

    virtual void onInit() {
        node = getSession()->getScene()->getRoot()->createChild();
        node->read("assets\\protag.so");
        auto stack = node->find<AnimationStack>();
        stack->play(0, "Bind");
        
        node->getTransform()->rotate(-1.1f, gfxm::vec3(0,1,0));
        node->getTransform()->translate(1,0,0);
    }
    virtual void onCleanup() {
        
    }

    virtual void onUpdate() {
        node->getTransform()->translate(
            gfxm::vec3(
                velo_dir.x * 1/60.0f,
                velo_dir.y * 1/60.0f,
                .0f
            )
        );
    }
};

class actorTicker : public Actor {
public:
    virtual void onInit() {
    }
    virtual void onUpdate() {
        static int i = 0;
        ++i;
        //std::cout << i << "\n";
    }
};

class sessGameplay : public GameSession {
    actorPlayerShip* playerShip;
public:
    virtual void onStart() {
        
        //playerShip = (actorPlayerShip*)createActor("actorPlayerShip");
        playerShip = createActor<actorPlayerShip>();
        createActor<actorTicker>();
        //createActor<actorTpsCamera>();
        createActor<actorGameCamera>();

        auto node = getScene()->getRoot()->createChild();
        if(!node->read("assets\\level3.so")) {
            LOG_WARN("Failed to load level");
        }

        class sqGetAllObjects : public SceneQuery{};
        class sqRaycast : public SceneQuery{};
        sqGetAllObjects q;
        getScene()->query(q);
        if(q) {

        }
    }
};

#endif
