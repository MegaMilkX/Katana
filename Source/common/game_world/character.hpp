#ifndef KT_CHARACTER_HPP
#define KT_CHARACTER_HPP

#include "resource/motion.hpp"
#include <btBulletDynamicsCommon.h>

class ktACharacter {
    std::shared_ptr<Motion> motion;
    std::unique_ptr<btRigidBody> capsule_body;
    std::unique_ptr<btCapsuleShape> capsule_shape;

public:
    void setPosition(const gfxm::vec3& pos);
};
STATIC_RUN(ktACharacter) {
    rttr::registration::class_<ktACharacter>("Character")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif
