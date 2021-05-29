#ifndef GAME_MODE_HPP
#define GAME_MODE_HPP

#include <vector>
#include <set>

class ktGameMode {
public:
    virtual ~ktGameMode() {
    }

    void _start() {
        onStart();
    }
    void _update(float dt) {
        onUpdate(dt);
    }
    void _render() {
        onRender();
    }
    void _cleanup() {
        onCleanup();
    }

    virtual void onStart()   = 0;
    virtual void onUpdate(float dt)  = 0;
    virtual void onRender()  = 0;
    virtual void onCleanup() = 0;
};


#endif
