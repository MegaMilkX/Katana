#include "katana_impl.hpp"

#include "timer.hpp"

#include "../common/engine.hpp"

#include "../common/scene/controllers/render_controller.hpp"

#include "../common/resource/resource_tree.hpp"


int main(int argc, char **argv) {
    if(!katanaInit()) {
        LOG_ERR("Failed to init platform");
        return 0;
    }    

    KatanaImpl kt;
    timer frameTimer;
    float dt = 1.0f/60.0f;

    ktStartup(&kt);
    while(!platformIsShuttingDown()) {
        frameTimer.start();
        platformUpdate(dt);

        kt.update();

        platformSwapBuffers();
        dt = frameTimer.end();
        kt.update_time(dt);
    }

    katanaCleanup();
}