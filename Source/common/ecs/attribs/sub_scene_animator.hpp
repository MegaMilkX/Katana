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
    Motion motion_lcl;

    void setSkeleton(std::shared_ptr<Skeleton> skel);
    std::shared_ptr<Skeleton> getSkeleton();

    void onGui(ecsWorld* world, entity_id ent);
};


#endif
