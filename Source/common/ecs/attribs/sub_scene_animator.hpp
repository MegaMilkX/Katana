#ifndef ECS_SUB_SCENE_ANIMATOR_HPP
#define ECS_SUB_SCENE_ANIMATOR_HPP


#include "../attribute.hpp"

#include "../../resource/animation.hpp"

class BaseMotion {
public:
    virtual ~BaseMotion() {}
};

class SingleMotion : public BaseMotion {
public:
    std::shared_ptr<Animation> anim;
    float cursor;
};

class BlendTreeMotion2 : public BaseMotion {
public:
};

class ActionGraphMotion : public BaseMotion {
public:
};

class LayeredMotion : public BaseMotion {
public:
    std::vector<std::shared_ptr<BaseMotion>> layers;
};

class ecsSubSceneAnimator : public ecsAttrib<ecsSubSceneAnimator> {
public:
    std::shared_ptr<BaseMotion> motion;

    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::BeginCombo("skeleton", "...")) {
            ImGui::EndCombo();
        }
        if(ImGui::BeginCombo("motion", "SingleMotion")) {
            ImGui::Selectable("SingleMotion");
            ImGui::Selectable("MotionGraph");
            ImGui::Selectable("BlendTree");
            ImGui::EndCombo();
        }
    }
};


#endif
