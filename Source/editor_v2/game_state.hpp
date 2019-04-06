#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

#include "scene/game_scene.hpp"

#include "scene/character.hpp"

#include "scene/controllers/render_controller.hpp"
#include "scene/controllers/dynamics_ctrl.hpp"

class GameState {
public:
    GameState() {
        scene.reset(new GameScene());
        vp.init(640, 480);
        dd.init();
    }
    ~GameState() {
        dd.cleanup();
    }

    void reset() {
    }

    void update(unsigned x, unsigned y) {
        dd.clear();
        scene->getController<DynamicsCtrl>()->setDebugDraw(&dd);
        scene->update();

        gfxm::mat4 proj;
        gfxm::mat4 view;
        CmCamera* cam = scene->getController<RenderController>()->getDefaultCamera();
        if(cam) {
            proj = cam->getProjection(x, y);
            view = cam->getView();
        } else {
            proj = gfxm::perspective(gfxm::radian(45.0f), x/(float)y, 0.01f, 1000.0f);
            view = gfxm::inverse(gfxm::mat4(1.0f));
        }

        vp.resize(x, y);

        DrawList dl;
        scene->getController<RenderController>()->getDrawList(dl);
        renderer.draw(&vp, proj, view, dl, true);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        scene->debugDraw(dd);
        //dd.draw(proj, view);
    }

    GameScene* getScene() {
        return scene.get();
    }
private:
    std::shared_ptr<GameScene> scene;

    RenderViewport vp;
    Renderer renderer;
    DebugDraw dd;
};

#endif
