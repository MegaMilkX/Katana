#include "../common/platform/platform.hpp"
#include "editor.hpp"

Editor* editor = 0;

int main() {
    if(!platformInit()) {
        LOG_ERR("Failed to initialize platform");
        return 0;
    }

    editor = new Editor();
    editor->Init();

    while(!platformIsShuttingDown()) {
        platformUpdate();
        editor->Update((GLFWwindow*)platformGetGlfwWindow());
        platformSwapBuffers();
    }

    editor->Cleanup();
    delete editor;
    
    platformCleanup();
    return 0;
}