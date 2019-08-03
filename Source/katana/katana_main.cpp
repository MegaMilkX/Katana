#include "katana_impl.hpp"

#include "timer.hpp"

#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include "../common/scene/controllers/render_controller.hpp"

#include "../common/resource/resource_tree.hpp"


int main(int argc, char **argv) {
    if(!platformInit()) {
        LOG_ERR("Failed to init platform");
    }    

    KatanaImpl kt;
    timer frameTimer;

    ktStartup(&kt);
    while(!platformIsShuttingDown()) {
        frameTimer.start();
        platformUpdate();

        kt.update();

        platformSwapBuffers();
        kt.update_time(frameTimer.end());
    }

    platformCleanup();
}