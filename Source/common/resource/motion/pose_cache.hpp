#ifndef MOTION_POSE_CACHE_HPP
#define MOTION_POSE_CACHE_HPP

#include "../skeleton.hpp"


typedef int motion_pose_id;
static const motion_pose_id MOTION_POSE_INVALID_ID = -1;

class MotionPoseCache {
    Skeleton* skel = 0;

public:
    void           init    (Skeleton* skeleton);

    void           reserve (int count);
    void           clear   ();

    motion_pose_id acquire ();
    void           release (motion_pose_id id);

};


#endif
