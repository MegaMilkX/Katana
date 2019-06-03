
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

    sess.start();
    while(!platformIsShuttingDown()) {
        platformUpdate();
        
        sess.update();
        DrawList dl;
        renderer.draw(&vp, gfxm::perspective(1.4f, 16.0f/9.0f, 0.01f, 1000.0f), gfxm::mat4(1.0f), dl, true);

        platformSwapBuffers();
    }
    sess.stop();

    platformCleanup();
}
