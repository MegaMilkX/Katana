#ifndef ECS_SUB_SCENE_ANIMATOR_HPP
#define ECS_SUB_SCENE_ANIMATOR_HPP


#include "../attribute.hpp"

#include "../../resource/animation.hpp"

#include "../../util/imgui_helpers.hpp"

enum MOTION_TYPE2 {
    MOTION_UNKNOWN2,
    MOTION_SINGLE,
    MOTION_BLENDTREE,
    MOTION_ACTIONGRAPH,
    MOTION_LAYERED
};

inline const char* getMotionTypeName(MOTION_TYPE2 type) {
    const char* cstr = 0;
    switch(type) {
    case MOTION_SINGLE: cstr = "SingleMotion"; break;
    case MOTION_BLENDTREE: cstr = "BlendTree"; break;
    case MOTION_ACTIONGRAPH: cstr = "ActionGraph"; break;
    case MOTION_LAYERED: cstr = "MotionLayers"; break;
    default: cstr = "Unknown"; break;
    }
    return cstr;
}

class BaseMotion {
public:
    virtual ~BaseMotion() {}
    virtual MOTION_TYPE2 getMotionType() const { return MOTION_UNKNOWN2; }
    virtual void onGui() {}
};

class SingleMotion : public BaseMotion {
public:
    std::shared_ptr<Animation> anim;
    float cursor;

    MOTION_TYPE2 getMotionType() const override { return MOTION_SINGLE; }
    
    void onGui() override {
        imguiResourceTreeCombo("anim clip", anim, "anm", [](){

        });
        ImGui::DragFloat(MKSTR("cursor###" << this).c_str(), &cursor);
    }
};

class BlendTreeMotion2 : public BaseMotion {
public:
    MOTION_TYPE2 getMotionType() const override { return MOTION_BLENDTREE; }
};

class ActionGraphMotion : public BaseMotion {
public:
    MOTION_TYPE2 getMotionType() const override { return MOTION_ACTIONGRAPH; }
};

class LayeredMotion : public BaseMotion {
public:
    std::vector<std::shared_ptr<BaseMotion>> layers;

    MOTION_TYPE2 getMotionType() const override { return MOTION_LAYERED; }
};

class ecsSubSceneAnimator : public ecsAttrib<ecsSubSceneAnimator> {
public:
    std::shared_ptr<BaseMotion> motion;

    void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::BeginCombo("skeleton", "...")) {
            ImGui::EndCombo();
        }

        std::string currentMotionName = "<null>";
        if(motion) {
            currentMotionName = getMotionTypeName(motion->getMotionType());
        }
        if(ImGui::BeginCombo("motion", currentMotionName.c_str())) {
            if(ImGui::Selectable("SingleMotion", (motion && (motion->getMotionType() == MOTION_SINGLE)))) {
                motion.reset(new SingleMotion());
            }
            if(ImGui::Selectable("BlendTree", (motion && (motion->getMotionType() == MOTION_BLENDTREE)))) {
                motion.reset(new BlendTreeMotion2());
            }
            if(ImGui::Selectable("ActionGraph", (motion && (motion->getMotionType() == MOTION_ACTIONGRAPH)))) {
                motion.reset(new ActionGraphMotion());
            }
            if(ImGui::Selectable("Layers", (motion && (motion->getMotionType() == MOTION_LAYERED)))) {
                motion.reset(new LayeredMotion());
            }
            ImGui::EndCombo();
        }
    }
};


#endif
