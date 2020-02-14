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
    class iterator {
        MotionBlackboard* bb = 0;
        int idx = -1;
    public:
        iterator() {}
        iterator(MotionBlackboard* bb, int idx)
        : bb(bb), idx(idx) {}

        int getIndex() const {
            return idx;
        }

        iterator& operator++() {
            while(idx < bb->params.size()) {
                ++idx;
                if(!bb->free_slots.count(idx)) {
                    break;
                }
            }
            return *this;
        }

        operator bool() const {
            if(idx < 0 || idx >= bb->params.size() || !bb) {
                return false;
            }
            if(bb->free_slots.count(idx)) {
                return false;
            }
            return true;
        }

        bool operator<(const iterator& other) const {
            return idx < other.idx;
        }
        MotionParamData& operator->() {
            return bb->params[idx];
        }
        MotionParamData& operator*() {
            return bb->params[idx];
        }

    };

    iterator begin() {
        int idx = 0;
        for(int i = 0; i < params.size(); ++i) {
            if(free_slots.count(i) == 0) {
                idx = i;
                break;
            }
        }
        return iterator(this, idx);
    }
    iterator end() {
        return iterator(this, -1);
    }

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
        animator->setMotion(this); // TODO: Move to constructor
        if(skeleton) {
            animator->setSkeleton(skeleton);
        }
    }
    AnimatorBase* getAnimator() {
        return animator.get();
    }
    MotionBlackboard& getBlackboard() {
        return blackboard;
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
