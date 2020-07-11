#ifndef ECS_ANIMATOR_HPP
#define ECS_ANIMATOR_HPP


#include "../attribute.hpp"
#include "../../resource/motion.hpp"
#include "../../util/imgui_helpers.hpp"


class ecsysAnimation;
class ecsAnimator : public ecsAttrib<ecsAnimator> {
friend ecsysAnimation;
    struct AnimNode {
        ecsTranslation* t = 0;
        ecsRotation*    r = 0;
        ecsScale*       s = 0;
    };
    
    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<Motion>   motion_ref;
    Motion                    motion_lcl;

    AnimSampleBuffer          sample_buffer;
    std::vector<AnimNode>     target_nodes;

public:

    void setSkeleton(std::shared_ptr<Skeleton> skel) {
        skeleton = skel;
        if(motion_ref) {
            setMotion(motion_ref);
        }
    }
    void setMotion(std::shared_ptr<Motion> motion) {
        motion_ref = motion;
        motion_lcl = Motion();
        if(!motion_ref) {
            return;
        }

        dstream strm;
        motion_ref->serialize(strm);
        strm.jump(0);
        motion_lcl.deserialize(strm, strm.bytes_available());

        if(skeleton) {
            motion_lcl,skeleton = skeleton;
            motion_lcl.rebuild();
        }

        getEntityHdl().signalUpdate<ecsAnimator>();
    }
    Skeleton* getSkeleton() { return skeleton.get(); }
    Motion* getLclMotion() { return &motion_lcl; }


    void write(ecsWorldWriteCtx& out) override {
        out.writeResource(skeleton);
        out.writeResource(motion_ref);
    }
    void read(ecsWorldReadCtx& in) override {
        skeleton = in.readResource<Skeleton>();
        motion_ref = in.readResource<Motion>();

        if(skeleton && motion_ref) {
            setSkeleton(skeleton);
            setMotion(motion_ref);
        }
    }

    void onGui(ecsWorld* world, entity_id ent) override {
        imguiResourceTreeCombo("skeleton", skeleton, "skl", [this](){
            setSkeleton(skeleton);
        });
        imguiResourceTreeCombo("motion", motion_ref, "motion", [this](){
            setMotion(motion_ref);
        });
    }

};


#endif
