#include <iostream>
#include "util/log.hpp"
#include <glfw/glfw3.h>

//#include "input/input_device_glfw_kb_mouse.hpp"
#include "editor.hpp"
#include "game.hpp"
#include "editor_state.hpp"

GLFWwindow* window = 0;
Editor* editor = 0;

void cbWindowResize(GLFWwindow *, int w, int h) {
    
}

bool initWindow() {
    if(!glfwInit())
    {
        std::cout << "glfwInit() failed" << std::endl;
        return false;
    }
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    
    window = glfwCreateWindow(1280, 720, "Aurora3", NULL, NULL);
    if(!window)
    {
        glfwTerminate();
        std::cout << "failed to create a window" << std::endl;
        return false;
    }
    glfwSetFramebufferSizeCallback(window, &cbWindowResize);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    WGLEXTLoadFunctions();
    GLEXTLoadFunctions();

    LOG("GL_VENDOR    : " << glGetString(GL_VENDOR));
    LOG("GL_RENDERER  : " << glGetString(GL_RENDERER));
    LOG("GL_VERSION   : " << glGetString(GL_VERSION));
    LOG("GLSL_VERSION : " << glGetString(GL_SHADING_LANGUAGE_VERSION));

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    GL_LOG_ERROR("");

    return true;
}

void cleanupWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

#include "util/filesystem.hpp"
#include "resource/data_registry.h"

void initFilesystemResources(const std::string& rootDir) {
    std::vector<std::string> files =
        find_all_files(rootDir, "*.scn;*.geo;*.anim;*.mat;*.png;*.jpg;*.jpeg");
    std::vector<std::string> resNames = files;
    for(auto& f : resNames) {
        f.erase(f.find_first_of(rootDir), rootDir.size());
        if(f[0] == '\\') f.erase(0, 1);
        std::replace(f.begin(), f.end(), '\\', '/');
    }

    for(size_t i = 0; i < files.size(); ++i) {
        GlobalDataRegistry().Add(
            resNames[i],
            DataSourceRef(new DataSourceFilesystem(files[i]))
        );
    }
}

#include "input/input_mgr.hpp"
#include "input/input_glfw.hpp"

int main() {
    initFilesystemResources(get_module_dir());
    if(!initWindow()) {
        return 0;
    }

    /*
    InputKeyboardMouseWin32* keyboardWin32 = new InputKeyboardMouseWin32(window);
    gInput.AddDevice(keyboardWin32);
    gInput.Init();
    */

    input().getTable().load(get_module_dir() + "\\bindings.json");
    initGlfwInputCallbacks(window, &input());

    Game game;
    game.init();

    editor = new Editor();
    editor->Init();

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        updateGlfwInput();

        editor->Update(window);
        glfwSwapBuffers(window);
    }

    editor->Cleanup();
    delete editor;

    game.cleanup();

    cleanupWindow();
    return 0;
}