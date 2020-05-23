#ifndef ECS_SUB_SCENE_ANIMATOR_HPP
#define ECS_SUB_SCENE_ANIMATOR_HPP


#include "../attribute.hpp"

#include "../../resource/motion.hpp"

#include "../../util/imgui_helpers.hpp"


class ecsTupleAnimatedSubScene;
class ecsSubSceneAnimator : public ecsAttrib<ecsSubSceneAnimator> {    
    std::shared_ptr<Skeleton> skeleton;

public:
    ecsTupleAnimatedSubScene* tuple = 0;
    std::shared_ptr<Motion> motion_ref;
    Motion motion_lcl_;
    Motion* motion_ptr = 0;

    ecsSubSceneAnimator();

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    std::shared_ptr<Skeleton> getSkeleton();
    void setMotion(std::shared_ptr<Motion> motion, bool dont_use_local_motion_object = false /* only for tools */);
    Motion* getLclMotion();

    void onGui(ecsWorld* world, entity_id ent);

    void write(ecsWorldWriteCtx& out) override;
    void read(ecsWorldReadCtx& in) override;
};


#endif
