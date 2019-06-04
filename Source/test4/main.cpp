
void getDrawList();


void startSession();
void endSession();


void updateBehaviors();
void updateDynamics();
void updateCollision();
void updateAudio();
void updateConstraints();
void renderToScreen();

#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include <iostream>
#include "core/session.hpp"
#include "sess_gameplay.hpp"

#include "../common/renderer.hpp"
#include "../common/render_viewport.hpp"

void main() {
    if(!platformInit()) {
        LOG_ERR("Failed to init platform");
    }

    RenderViewport vp;
    vp.init(1280, 720);
    Renderer renderer;
    

    Scene scene;

    sessGameplay sess;
    sess.setScene(&scene);

    std::shared_ptr<Mesh> mesh = retrieve<Mesh>("test.msh");

    DrawList dl;
    dl.add(DrawList::Solid{
        mesh->mesh.getVao(), 0,
        mesh->indexCount(),
        gfxm::mat4(1.0f)
    });

    sess.start();
    while(!platformIsShuttingDown()) {
        platformUpdate();
        
        sess.update();
        renderer.draw(
            &vp, 
            gfxm::perspective(1.4f, 16.0f/9.0f, 0.01f, 1000.0f), 
            gfxm::inverse(gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(.0f, .0f, 5.0f))), 
            dl, 
            true
        );

        platformSwapBuffers();
    }
    sess.stop();

    platformCleanup();
}
