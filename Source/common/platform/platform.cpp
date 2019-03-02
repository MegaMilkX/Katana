#include "platform.hpp"

#include <glfw/glfw3.h>

#include "../shader_factory.hpp"
#include "../event.hpp"
#include "../debug_draw.hpp"
#include "../util/init_filesystem_resources.hpp"
#include "../input/input_mgr.hpp"
#include "../input/input_glfw.hpp"

static GLFWwindow* window = 0;

bool initWindow();
void cleanupWindow();

bool platformInit() {
    initFilesystemResources(get_module_dir());
    if(!initWindow()) {
        return 0;
    }
    ShaderFactory::init();

    DebugDraw::getInstance()->init();

    input().getTable().load(get_module_dir() + "\\bindings.json");
    initGlfwInputCallbacks(window, &input());
    return true;
}

void platformCleanup() {
    DebugDraw::getInstance()->cleanup();
    ShaderFactory::cleanup();
    cleanupWindow();
}

bool platformIsShuttingDown() {
    return glfwWindowShouldClose(window);
}

void platformUpdate() {
    glfwPollEvents();
    updateGlfwInput(window);
    eventMgr().pollEvents();
}

void platformSwapBuffers() {
    glfwSwapBuffers(window);
}

void platformGetViewportSize(unsigned& width, unsigned& height) {
    int w = 0, h = 0;
    glfwGetWindowSize(window, &w, &h);
    width = (unsigned)w;
    height = (unsigned)h;
}

void platformGetMousePos(unsigned& x, unsigned& y) {
    double cursor_x, cursor_y;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);
    x = (unsigned)cursor_x;
    y = (unsigned)cursor_y;
}

void* platformGetGlfwWindow() {
    return (void*)window;
}

static void cbWindowResize(GLFWwindow *, int w, int h) {
    
}

// ==== Window =======================================

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