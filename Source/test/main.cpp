#include "../common/platform/platform.hpp"
#include "../common/util/log.hpp"

#include "app_state/app_state.hpp"

AppState* _app_state = 0;

#include "test_state.hpp"

int main() {
    if(!platformInit()) {
        LOG_ERR("Failed to initialize platform");
        return 0;
    }

    TestState tstt;

    _app_state = &tstt;

    _app_state->onInit();
    while(!platformIsShuttingDown()) {
        platformUpdate();
        unsigned w, h;
        unsigned cx, cy;
        platformGetViewportSize(w, h);
        platformGetMousePos(cx, cy);
		
        if(_app_state) {
            _app_state->onUpdate();
            _app_state->onGui();
            _app_state->onRender();
        }
		
        platformSwapBuffers();
    }
    _app_state->onCleanup();

    platformCleanup();
    return 0;
}