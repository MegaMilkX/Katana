#include "game.hpp"

#include "gfxm.hpp"

void Game::init() {
    scene = Scene::create();
    renderer.setScene(scene);
    animator_sys.setScene(scene);
    behavior_sys.setScene(scene);
}

void Game::cleanup() {
    scene->destroy();
}

Scene* Game::getScene() {
    return scene;
}

void Game::update(int w, int h) {
    animator_sys.Update(1.0f / 60.0f);
    behavior_sys.update();

    gfxm::mat4 proj;
    gfxm::mat4 view;

    Camera* cam = scene->getCurrentCamera();
    if(cam) {
        proj = cam->getProjection(w, h);
        view = cam->getView();
    } else {
        proj = gfxm::perspective(0.90f, w/(float)h, 0.01f, 1000.0f);
        view = gfxm::inverse(gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(0.0f, 0.0f, 5.0f)));
    }

    renderer.draw(0, w, h, proj, view);
}