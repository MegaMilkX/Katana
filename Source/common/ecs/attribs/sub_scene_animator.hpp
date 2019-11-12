#ifndef ECS_SUB_SCENE_ANIMATOR_HPP
#define ECS_SUB_SCENE_ANIMATOR_HPP


#include "../attribute.hpp"

class ecsSubSceneAnimator : public ecsAttrib<ecsSubSceneAnimator> {
public:
    
    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::BeginCombo("skeleton", "...")) {
            ImGui::EndCombo();
        }
        if(ImGui::BeginCombo("motion", "SingleMotion")) {
            ImGui::EndCombo();
        }
    }
};


#endif
