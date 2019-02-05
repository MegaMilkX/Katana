#include "game.hpp"

#include "gfxm.hpp"

#include "debug_draw.hpp"

void Game::init() {
    scene = Scene::create();
    renderer.setScene(scene);
    g_buffer.resize(1280, 720);

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
    DebugDraw::getInstance()->clear();

    if(g_buffer.getWidth() != w || g_buffer.getHeight() != h) {
        g_buffer.resize(w, h);
    }

    animator_sys.Update(1.0f / 60.0f);
    behavior_sys.update();
    scene->getSceneComponent<PhysicsWorld>()->update(1.0f / 60.0f);

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

    renderer.draw(&g_buffer, 0, w, h, proj, view);
    //scene->getSceneComponent<PhysicsWorld>()->debugDraw();

    DebugDraw::getInstance()->draw(proj, view);
}