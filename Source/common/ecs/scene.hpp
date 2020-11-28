#ifndef KT_SCENE_HPP
#define KT_SCENE_HPP

#include <btBulletDynamicsCommon.h>

class ktModel {

};

class ktScene {
    btDefaultCollisionConfiguration* collisionConf;
    btCollisionDispatcher* dispatcher;
    btDbvtBroadphase* broadphase;
    btSequentialImpulseConstraintSolver* constraintSolver;
    btDiscreteDynamicsWorld* world;

public:
    
};


#endif
