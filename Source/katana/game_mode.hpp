#ifndef GAME_MODE_HPP
#define GAME_MODE_HPP

#include "actor.hpp"
#include "../common/world.hpp"

#include <vector>
#include <set>

class ktGameMode {
    RTTR_ENABLE()

    ktWorld world;
public:
    virtual ~ktGameMode() {
    }

    ktWorld& getWorld() { return world; }
    GameScene& getScene() { return *world.getScene(); }



    void _start() {
        world.start();
        onStart();
    }
    void _update(float dt) {
        world.update(dt);
        onUpdate();
    }
    void _cleanup() {
        world.cleanup();
        onCleanup();
    }

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
