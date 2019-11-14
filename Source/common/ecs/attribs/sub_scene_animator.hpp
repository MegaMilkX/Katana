#ifndef ECS_SUB_SCENE_ANIMATOR_HPP
#define ECS_SUB_SCENE_ANIMATOR_HPP


#include "../attribute.hpp"

#include "../../resource/animation.hpp"

#include "../../util/imgui_helpers.hpp"

class FloatBlackboard {
    std::map<std::string, float> map;
public:

};

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
protected:
    std::vector<AnimSample> samples;
    std::shared_ptr<Skeleton> skeleton;

public:
    virtual ~BaseMotion() {}
    virtual MOTION_TYPE2 getMotionType() const { return MOTION_UNKNOWN2; }
    virtual void onGui() {}
    virtual void onSkeletonChange() = 0;

    virtual void update(float dt) = 0;

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        samples.resize(skeleton->boneCount());
        onSkeletonChange();
    }
    const std::vector<AnimSample>& getSamples() const {
        return samples;
    }
};

class SingleMotion : public BaseMotion {
public:
    std::shared_ptr<Animation> anim;
    std::vector<int32_t> mapping;
    float cursor = 0.0f;

    MOTION_TYPE2 getMotionType() const override { return MOTION_SINGLE; }
    
    void onGui() override {
        imguiResourceTreeCombo("anim clip", anim, "anm", [this](){
            cursor = .0f;
            onSkeletonChange();
        });
        ImGui::DragFloat(MKSTR("cursor###" << this).c_str(), &cursor);
    }

    void update(float dt) override {
        if(!anim || !skeleton) {
            return;
        }
        anim->sample_remapped(samples, cursor, mapping);
        cursor += dt * anim->fps;
        if(cursor>=anim->length) {
            cursor -= anim->length;
        }
    }

    void onSkeletonChange() override {
        if(anim && skeleton) {
            mapping = anim->getMapping(skeleton.get());
        }
    }
};

class BlendTreeMotion2 : public BaseMotion {
public:
    MOTION_TYPE2 getMotionType() const override { return MOTION_BLENDTREE; }

    void update(float dt) override {
        // TODO: Get samples
    }

    void onSkeletonChange() override {
        
    }
};

class ActionGraphMotion : public BaseMotion {
public:
    MOTION_TYPE2 getMotionType() const override { return MOTION_ACTIONGRAPH; }

    void update(float dt) override {
        // TODO: Get samples
    }

    void onSkeletonChange() override {
        
    }
};

class LayeredMotion : public BaseMotion {
public:
    std::vector<std::shared_ptr<BaseMotion>> layers;

    MOTION_TYPE2 getMotionType() const override { return MOTION_LAYERED; }

    void update(float dt) override {
        // TODO: Get samples
    }

    void onSkeletonChange() override {
        
    }
};

class ecsTupleAnimatedSubScene;
class ecsSubSceneAnimator : public ecsAttrib<ecsSubSceneAnimator> {    
    std::shared_ptr<Skeleton> skeleton;

public:
    ecsTupleAnimatedSubScene* tuple = 0;
    std::shared_ptr<BaseMotion> motion;

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    std::shared_ptr<Skeleton> getSkeleton();

    void onGui(ecsWorld* world, entity_id ent);
};


#endif
