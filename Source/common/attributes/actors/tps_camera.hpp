#ifndef KT_TPS_CAMERA_HPP
#define KT_TPS_CAMERA_HPP

#include "../../world.hpp"
#include "../../input_listener.hpp"

#include "../camera.hpp"

#include "../../platform/platform.hpp"

class actorTpsCamera : public ktActor, public InputListenerWrap {
    RTTR_ENABLE(ktActor)

    gfxm::vec3 pivot = gfxm::vec3(0.0f, 1.3f, 0.0f);
    float distance = 10.0f;
    float angle_y = 0.0f;
    float angle_x = 0.0f;
    float _distance = 10.0f;
    float _angle_y = 0.0f;
    float _angle_x = 0.0f;

    Camera* cam = 0;
    ktWorld* wrld = 0;
public:
    void onStart(ktWorld* world) override {
        platformMouseSetEnabled(false);

        cam = getOwner()->get<Camera>().get();
        //cam->setZFar(500.0f);
        //cam->setZNear(0.1f);
        //cam->setFov(0.76f);

        wrld = world;

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
    void onUpdate() override {
        gfxm::vec3 tgt_pos;
        TransformNode* t = 0;
        float height = 1.6f;

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
        if(wrld->getPhysics()->sweepSphere(
            sweepRad,
            pivot,
            pivot + tcam.back() * _distance,
            hit
        )) {
            float len = gfxm::length(pivot - hit);
            _distance = len;
        }

        tcam.translate(tcam.back() * _distance);

        getOwner()->getTransform()->setPosition(pivot);
        getOwner()->getTransform()->translate(getOwner()->getTransform()->back() * _distance);
        getOwner()->getTransform()->setRotation(gfxm::angle_axis(_angle_y, gfxm::vec3(.0f, 1.0f, .0f)));
        getOwner()->getTransform()->rotate(_angle_x, getOwner()->getTransform()->right());
        //getOwner()->getTransform()->setTransform(tcam.matrix());
    }
    void onCleanup() override {
        platformMouseSetEnabled(true);
    }
};
REG_ATTRIB(actorTpsCamera, TpsCameraActor, Actors);

#endif
