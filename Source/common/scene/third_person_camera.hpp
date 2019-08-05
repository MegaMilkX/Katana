#ifndef TPS_CAMERA_HPP
#define TPS_CAMERA_HPP

#include "actor_object.hpp"
#include "../components/camera.hpp"
#include "../input_listener.hpp"

#include "../../common/util/serialization.hpp"

#include "character.hpp"

class ThirdPersonCamera : public ActorObject, public InputListenerWrap {
public:
    virtual void init() {
        get<Camera>();
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
    virtual void update() {
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
        if(getScene()->getController<DynamicsCtrl>()
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
        getTransform()->setTransform(tcam.matrix());
    }

    void setTargetCharacter(CharacterActor* actor) {
        tgt_chara = actor;
    }
    CharacterActor* getTargetCharacter() {
        return tgt_chara;
    }

    virtual void serialize(std::ostream& out) {
        std::string tgt_name = "";
        if(tgt_chara) {
            tgt_name = tgt_chara->getName();
        }
        wt_string(out, tgt_name);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        std::string tgt_name = rd_string(in);
        //auto o = getScene()->findObject<CharacterActor>(tgt_name);
        //setTargetCharacter(o);
        /*
        if(!o) {
            LOG_WARN("Failed to find character object '" << tgt_name << "' for tps camera");
        }*/
        // TODO
    }

    virtual void onGui() {
        ImGui::Text("Target character");
        auto chara = getTargetCharacter();
        ImGui::Button(chara ? chara->getName().c_str() : "empty");
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_OBJECT")) {
                ktNode* tgt_dnd_so = *(ktNode**)payload->Data;
                /*
                if(tgt_dnd_so->get_type().is_derived_from<CharacterActor>()) {
                    setTargetCharacter((CharacterActor*)tgt_dnd_so);
                } else {
                    LOG_WARN(tgt_dnd_so->getName() << " must be of CharacterActor type or derived");
                }*/
            }
            ImGui::EndDragDropTarget();
        }
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

#endif
