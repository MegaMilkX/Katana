#ifndef SESSION_HPP
#define SESSION_HPP

#include "actor.hpp"
#include "../common/scene/game_scene.hpp"

#include <vector>
#include <set>

class ktGameMode {
    RTTR_ENABLE()

    GameScene               scene;
public:
    virtual ~ktGameMode() {
    }

    GameScene*  getScene() { return &scene; }



    virtual void onStart() {}
    virtual void onUpdate() {}
    virtual void onCleanup() {}
};
STATIC_RUN(ktGameMode) {
    rttr::registration::class_<ktGameMode>("BasicGameMode")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif
