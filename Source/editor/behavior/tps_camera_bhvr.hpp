#ifndef TPS_CAMERA_BHVR_HPP
#define TPS_CAMERA_BHVR_HPP

#include "behavior.hpp"
#include "../components/camera.hpp"
#include "../input_listener.hpp"

#include "../../common/util/serialization.hpp"

#include "../scene/character.hpp"

class TpsCameraBhvr : public Behavior, public InputListenerWrap {
    RTTR_ENABLE(Behavior)
public:
    void onInit() {
        getOwner()->get<Camera>();
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
    void onUpdate() {
        gfxm::vec3 tgt_pos;
        TransformNode* t = 0;
        float height = 1.0f;
        if(tgt_chara) {
            t = tgt_chara->getTransform();
            height = tgt_chara->getCameraHeight();
        }

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
        if(getOwner()->getScene()->getController<DynamicsCtrl>()
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
        getOwner()->getTransform()->setTransform(tcam.matrix());
    }

    void onGui() {
        ImGui::Text("TpsCam");
    }

    void onRegister() {
        rttr::registration::class_<TpsCameraBhvr>("TpsCameraBhvr")
            .constructor<>()(
                rttr::policy::ctor::as_raw_ptr
            );
    }

    void write(out_stream& out) {

    }
    void read(in_stream& in) {
        
    }
private:
    gfxm::vec3 pivot = gfxm::vec3(0.0f, 1.3f, 0.0f);
    float distance = 10.0f;
    float angle_y = 0.0f;
    float angle_x = 0.0f;
    float _distance = 10.0f;
    float _angle_y = 0.0f;
    float _angle_x = 0.0f;

    CharacterActor* tgt_chara = 0;
};
REG_REFLECTABLE(TpsCameraBhvr);

#endif
