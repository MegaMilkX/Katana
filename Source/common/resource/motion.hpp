#ifndef MOTION_HPP
#define MOTION_HPP

#include "skeleton.hpp"

enum MOTION_TYPE {
    MOTION_UNKNOWN,
    MOTION_CLIP,
    MOTION_BLEND_TREE
};

inline const char* motionTypeToCStr(MOTION_TYPE t) {
    const char* ptr = 0;
    switch(t) {
    case MOTION_CLIP:
        ptr = "Animation Clip";
        break;
    case MOTION_BLEND_TREE:
        ptr = "Blend Tree";
        break;
    default:
        ptr = "Unknown";
        break;
    }
    return ptr;
}

class Motion {
protected:
    float cursor = .0f; // normalized
    std::vector<AnimSample> samples;
    std::shared_ptr<Skeleton> skeleton;
public:
    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        if(!skel) {
            return;
        }
        samples = skeleton->makePoseArray();

        onSkeletonChanged();
    }
    virtual void advance(float dt) = 0;
    virtual std::vector<AnimSample>& getPose() {
        return samples;
    }
    virtual MOTION_TYPE getType() const { return MOTION_UNKNOWN; }
    
    virtual void onSkeletonChanged() {}
    virtual void onGui() {}

    virtual void write(out_stream& out) {}
    virtual void read(in_stream& in) {}
};

#endif
