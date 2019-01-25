#ifndef GAME_HPP
#define GAME_HPP

#include "scene.hpp"
#include "renderer.hpp"
#include "animator_sys.hpp"
#include "behavior_system.hpp"
#include "camera.hpp"

class Game {
public:
    void init();
    void cleanup();

    Scene* getScene();

    void update(int w, int h);
private:
    Scene* scene = 0;
    Renderer renderer;
    AnimatorSys animator_sys;
    SysBehavior behavior_sys;
};

#endif