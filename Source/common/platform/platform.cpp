#include "platform.hpp"

#include <glfw/glfw3.h>

#include "../shader_factory.hpp"
#include "../mesh_pool.hpp"
//#include "../event.hpp"
#include "../debug_draw.hpp"
//#include "../util/init_filesystem_resources.hpp"
#include "../resource/resource_tree.hpp"
#include "../input/input_mgr.hpp"
#include "../input/input_glfw.hpp"
#include "../audio.hpp"
#include "../lib/imgui_wrap.hpp"
#include "../util/threading/utils.hpp"
#include "../input2/input2.hpp"
#include "../angel_script.hpp"

static GLFWwindow* window = 0;
static GLFWwindow* async_context = 0;

bool initWindow(PlatformStartupParams* params);
void cleanupWindow();

static ImFont* font1;

Config& platformGetConfig() {
    static Config cfg;
    return cfg;
}

bool platformInit(PlatformStartupParams* params) {
    // Set this thread as the main one (used for deferred calls that need to be executed on the main thread)
    setMainThread(std::this_thread::get_id());
    // Fill the resource register
    gResourceTree.scanFilesystem(get_module_dir() + "/" + platformGetConfig().data_dir);
    
    if(!initWindow(params)) {
        return 0;
    }
    ShaderFactory::init();
    MeshPool::init();

    DebugDraw::getInstance()->init();

    input().getTable().load(get_module_dir() + "\\bindings.json");
    initGlfwInputCallbacks(window, &input());

    audio().init(44100, 16);
    
    // Scripting
    aslInit();

    // ImGui ========
    ImGuiInit();
    auto& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    //io.Fonts->AddFontFromMemoryCompressedTTF()
    //font1 = io.Fonts->AddFontFromFileTTF((get_module_dir() + "\\Karla-Regular.ttf").c_str(), 14);
    //io.Fonts->Build();
    // ==============

    return true;
}

void platformCleanup() {
    ImGuiCleanup();

    aslCleanup();
    audio().cleanup();
    DebugDraw::getInstance()->cleanup();
    MeshPool::cleanup();
    ShaderFactory::cleanup();
    cleanupWindow();
}

bool platformIsShuttingDown() {
    return glfwWindowShouldClose(window);
}

void platformUpdate(float dt) {
    glfwPollEvents();
    updateGlfwInput(window);
    inputReadDevices();
    inputUpdate(dt);
    //eventMgr().pollEvents();

    unsigned w, h;
    unsigned cx, cy;
    platformGetViewportSize(w, h);
    platformGetMousePos(cx, cy);
    ImGuiUpdate(dt, w, h);
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2((float)cx, (float)cy);
}

void platformSwapBuffers() {
    ImGuiDraw();

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

void platformMakeAsyncRenderContextCurrent() {
    glfwMakeContextCurrent(async_context);
}

void platformMouseSetEnabled(bool v) {
    glfwSetInputMode(window, GLFW_CURSOR, v ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

static void cbWindowResize(GLFWwindow *, int w, int h) {
    
}

// ==== Window =======================================

bool initWindow(PlatformStartupParams* params) {
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
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    if(params) {
        if(params->hide_window) {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }
    }
    
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

    {
        GLint i;
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &i);
        LOG("GL_MAX_UNIFORM_BUFFER_BINDINGS: " << i);
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &i);
        LOG("GL_MAX_UNIFORM_BLOCK_SIZE: " << i);
        glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &i);
        LOG("GL_MAX_VERTEX_UNIFORM_BLOCKS: " << i);
        glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &i);
        LOG("GL_MAX_FRAGMENT_UNIFORM_BLOCKS: " << i);
        glGetIntegerv(GL_MAX_GEOMETRY_UNIFORM_BLOCKS, &i);
        LOG("GL_MAX_GEOMETRY_UNIFORM_BLOCKS: " << i);
    }

    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glDisable(GL_LINE_SMOOTH);

    // Second context for multithreading
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    async_context = glfwCreateWindow(640, 480, "", NULL, window);

    glfwMakeContextCurrent(window);

    return true;
}

void cleanupWindow() {
    glfwDestroyWindow(async_context);
    glfwDestroyWindow(window);
    glfwTerminate();
}