#ifndef MOTION_HPP
#define MOTION_HPP

#include "resource.h"
#include "anim_primitive.hpp"

#include "skeleton.hpp"
#include "../resource/entity_template.hpp"
#include "../resource/animation.hpp"

#include <memory>

struct MotionParamData {
    std::string     name;
    float           value = .0f;
    float           target_value = .0f;
    float           lerp_step = .0f; // per second
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
        assert(id >= 0);
        params[id].name = name;
    }
    void setValue(int id, float val) {
        if (id < 0) return;
        params[id].value = val;
    }
    void lerpValue(int id, float target, float time) {
        if (id < 0) return;
        params[id].target_value = target;
        params[id].lerp_step = (params[id].target_value - params[id].value) / time;
    }
    const std::string& getName(int id) {
        return params[id].name;
    }
    int getIndex(const char* name) {
        for(int i = 0; i < params.size(); ++i) {
            auto& p = params[i];
            if(p.name == name) {
                return i;
            }
        }
        return -1;
    }
    float getValue(int id) {
        return params[id].value;
    }
    int getOrCreate(const char* name) {
        for(int i = 0; i < params.size(); ++i) {
            auto& p = params[i];
            if(p.name == name) {
                return i;
            }
        }
        int id = allocValue();
        setName(id, name);
        return id;
    }

    void update(float dt) {
        for(int i = 0; i < params.size(); ++i) {
            auto& p = params[i];
            if(p.lerp_step != .0f) {
                float step = .0f;
                if(abs(p.target_value - p.value) < abs(p.lerp_step * dt)) {
                    p.value = p.target_value;
                    p.lerp_step = .0f;
                } else {
                    p.value += p.lerp_step * dt;
                }
            }
        }
    }

    void serialize(out_stream& out) {
        std::vector<MotionParamData*> actual_params;
        for(int i = 0; i < params.size(); ++i) {
            if(free_slots.count(i)) {
                continue;
            }
            actual_params.push_back(&params[i]);
        }

        out.write<uint32_t>(actual_params.size());
        for(int i = 0; i < actual_params.size(); ++i) {
            auto p = actual_params[i];
            out.write<uint32_t>(p->name.size());
            out.write(p->name.data(), p->name.size());
            out.write<float>(p->value);
        }
    }
    void deserialize(in_stream& in) {
        auto param_count = in.read<uint32_t>();

        for(int i = 0; i < param_count; ++i) {
            params.push_back(MotionParamData());
            auto name_len = in.read<uint32_t>();
            params.back().name.resize(name_len);
            in.read((void*)params.back().name.data(), name_len);
            params.back().value = in.read<float>();
        }
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

    std::shared_ptr<AnimatorBase> animator;
    MotionBlackboard              blackboard;

public:
    std::string                     reference_path;
    std::shared_ptr<Skeleton>       skeleton;

    void rebuild(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        rebuild();
    }
    void rebuild() {
        if(animator) {
            animator->rebuild();
        }
    }

    std::shared_ptr<Skeleton> getSkeleton() {
        return skeleton;
    }

    const char* getWriteExtension() const override { return "motion"; }

    template<typename ANIMATOR_T>
    void resetAnimator() {
        animator.reset(new ANIMATOR_T(this));
        animator->rebuild();
    }
    AnimatorBase* getAnimator() {
        return animator.get();
    }
    MotionBlackboard& getBlackboard() {
        return blackboard;
    }

    void update(float dt, AnimSampleBuffer& sample_buffer) {
        getBlackboard().update(dt);
        sample_buffer.getRootMotionDelta().t = gfxm::vec3(0,0,0);
        sample_buffer.getRootMotionDelta().r = gfxm::quat(0,0,0,1);
        if(animator) {
            animator->update(dt, sample_buffer);
        }
    }

    void serialize(out_stream& out) override;
    bool deserialize(in_stream& in, size_t sz) override;
};
STATIC_RUN(Motion) {
    rttr::registration::class_<Motion>("Motion")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
