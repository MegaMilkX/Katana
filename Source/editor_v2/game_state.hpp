#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "scene/game_scene.hpp"

#include "scene/character.hpp"

class GameState {
public:
    GameState() {
        scene.reset(new GameScene());
        vp.init(640, 480);
        gfx_mgr.setScene(scene.get());
        anim_mgr.setScene(scene.get());
    }

    void reset() {
        scene->resetActors();
    }

    void update(unsigned x, unsigned y) {
        anim_mgr.update(1.0f/60.0f);
        scene->update();

        gfxm::mat4 proj;
        gfxm::mat4 view;
        CmCamera* cam = scene->getDefaultCamera();
        if(cam) {
            proj = cam->getProjection(x, y);
            view = cam->getView();
        } else {
            proj = gfxm::perspective(gfxm::radian(45.0f), x/(float)y, 0.01f, 1000.0f);
            view = gfxm::inverse(gfxm::mat4(1.0f));
        }

        vp.resize(x, y);

        DrawList dl;
        gfx_mgr.getDrawList(dl);
        renderer.draw(&vp, proj, view, dl, true);
    }

    GameScene* getScene() {
        return scene.get();
    }
private:
    std::shared_ptr<GameScene> scene;

    RenderViewport vp;
    Renderer renderer;
    GfxSceneMgr gfx_mgr;
    AnimationSceneMgr anim_mgr;
};

#endif
