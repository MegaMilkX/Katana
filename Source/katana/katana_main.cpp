#include "katana_impl.hpp"

#include "timer.hpp"

#include "../common/engine.hpp"

#include "../common/scene/controllers/render_controller.hpp"

#include "../common/resource/resource_tree.hpp"

#include "../common/stats/frame_stats.hpp"

int main(int argc, char **argv) {
    if(!katanaInit()) {
        LOG_ERR("Failed to init platform");
        return 0;
    }

    KatanaImpl kt;
    ktTimer frameTimer;
    float dt = 1.0f/60.0f;

    ktStartup(&kt);
    while(!platformIsShuttingDown()) {
        frameTimer.start();
        platformUpdate(dt);

        kt.update();

        platformSwapBuffers();
        dt = frameTimer.end();
        kt.update_time(dt);
        gFrameStats.frame_time = dt;
        gFrameStats.time += dt;
        gFrameStats.frame_id++;
        gFrameStats.fps = 1.0f / dt;
    }

    katanaCleanup();
}