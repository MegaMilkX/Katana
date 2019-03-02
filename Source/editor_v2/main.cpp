#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include "editor.hpp"

int main() {
    if(!platformInit()) {
        LOG_ERR("Failed to initialize platform");
        return 0;
    }

    Editor editor;
    editor.init();
    while(!platformIsShuttingDown()) {
        platformUpdate();
        unsigned w, h;
        unsigned cx, cy;
        platformGetViewportSize(w, h);
        platformGetMousePos(cx, cy);
        editor.update(w, h, cx, cy);
        platformSwapBuffers();
    }
    editor.cleanup();

    platformCleanup();
    return 0;
}