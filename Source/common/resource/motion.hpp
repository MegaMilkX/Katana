#ifndef MOTION_HPP
#define MOTION_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "skeleton.hpp"
#include "../scene/game_scene.hpp"

#include <memory>

struct MotionParamData {
    std::string     name;
    float           value = .0f;
};

class MotionBlackboard;
class MotionParamHdl {
    MotionBlackboard* board;
    int param_id;
public:
    MotionParamHdl(MotionBlackboard* board, int id)
    : board(board), param_id(id) {}

    float get() const;
    void  set(float val);

};

class MotionBlackboard {
    std::vector<MotionParamData> params;
    std::set<int> free_slots;

public:
    int allocValue() {
        int id = 0;
        if(free_slots.empty()) {
            id = params.size();
            params.push_back(MotionParamData());
        } else {
            id = *free_slots.begin();
            free_slots.erase(free_slots.begin());
        }
        return id;
    }
    void freeValue(int id) {
        free_slots.insert(id);
    }

    void setName(int id, const char* name) {
        params[id].name = name;
    }
    void setValue(int id, float val) {
        params[id].value = val;
    }
    const std::string& getName(int id) {
        return params[id].name;
    }
    float getValue(int id) {
        return params[id].value;
    }

};

inline float MotionParamHdl::get() const { 
    return board->getValue(param_id); 
}
inline void  MotionParamHdl::set(float val) { 
    board->setValue(param_id, val);
}


class Motion : public Resource {
    RTTR_ENABLE(Resource)

    std::unique_ptr<AnimatorBase> animator;
    MotionBlackboard              blackboard;

public:
    std::shared_ptr<GameScene> reference;
    std::shared_ptr<Skeleton>  skeleton;

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        if(animator) {
            animator->setSkeleton(skel);
        }
    }

    const char* getWriteExtension() const override { return "motion"; }

    template<typename ANIMATOR_T>
    void resetAnimator() {
        animator.reset(new ANIMATOR_T());
        if(skeleton) {
            animator->setSkeleton(skeleton);
        }
    }
    AnimatorBase* getAnimator() {
        return animator.get();
    }

    void rebuild() {

    }

    void update(float dt, std::vector<AnimSample>& samples) {
        if(animator) {
            animator->update(dt, samples);
        }
    }
};
STATIC_RUN(Motion) {
    rttr::registration::class_<Motion>("Motion")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
