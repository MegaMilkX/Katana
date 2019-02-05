#ifndef INPUT_GLFW_HANDLER_HPP
#define INPUT_GLFW_HANDLER_HPP

#include <string>
#include <map>
#include <GLFW/glfw3.h>
#include "input_mgr.hpp"

int64_t getGlfwKey(const std::string& key) {
    static std::map<std::string, int64_t> keys = {
        {"KB_UNKNOWN", -1}, {"KB_SPACE", 32}, {"KB_APOSTROPHE", 39}, {"KB_COMMA", 44}, {"KB_MINUS", 45}, {"KB_PERIOD", 46}, {"KB_SLASH", 47}, {"KB_0", 48}, {"KB_1", 49},
        {"KB_2", 50}, {"KB_3", 51}, {"KB_4", 52}, {"KB_5", 53}, {"KB_6", 54}, {"KB_7", 55}, {"KB_8", 56}, {"KB_9", 57}, {"KB_SEMICOLON", 59}, {"KB_EQUAL", 61},
        {"KB_A", 65}, {"KB_B", 66}, {"KB_C", 67}, {"KB_D", 68}, {"KB_E", 69}, {"KB_F", 70}, {"KB_G", 71}, {"KB_H", 72}, {"KB_I", 73}, {"KB_J", 74}, {"KB_K", 75},
        {"KB_L", 76}, {"KB_M", 77}, {"KB_N", 78}, {"KB_O", 79}, {"KB_P", 80}, {"KB_Q", 81}, {"KB_R", 82}, {"KB_S", 83}, {"KB_T", 84}, {"KB_U", 85}, {"KB_V", 86},
        {"KB_W", 87}, {"KB_X", 88}, {"KB_Y", 89}, {"KB_Z", 90}, {"KB_LEFT_BRACKET", 91}, {"KB_BACKSLASH", 92}, {"KB_RIGHT_BRACKET", 93}, {"KB_GRAVE_ACCENT", 96},
        {"KB_WORLD_1", 161}, {"KB_WORLD_2", 162}, {"KB_ESCAPE", 256}, {"KB_ENTER", 257}, {"KB_TAB", 258}, {"KB_BACKSPACE", 259}, {"KB_INSERT", 260}, {"KB_DELETE", 261},
        {"KB_RIGHT", 262}, {"KB_LEFT", 263}, {"KB_DOWN", 264}, {"KB_UP", 265}, {"KB_PAGE_UP", 266}, {"KB_PAGE_DOWN", 267}, {"KB_HOME", 268}, {"KB_END", 269},
        {"KB_CAPS_LOCK", 280}, {"KB_SCROLL_LOCK", 281}, {"KB_NUM_LOCK", 282}, {"KB_PRINT_SCREEN", 283}, {"KB_PAUSE", 284}, {"KB_F1", 290}, {"KB_F2", 291}, {"KB_F3", 292},
        {"KB_F4", 293}, {"KB_F5", 294}, {"KB_F6", 295}, {"KB_F7", 296}, {"KB_F8", 297}, {"KB_F9", 298}, {"KB_F10", 299}, {"KB_F11", 300}, {"KB_F12", 301}, {"KB_F13", 302},
        {"KB_F14", 303}, {"KB_F15", 304}, {"KB_F16", 305}, {"KB_F17", 306}, {"KB_F18", 307}, {"KB_F19", 308}, {"KB_F20", 309}, {"KB_F21", 310}, {"KB_F22", 311},
        {"KB_F23", 312}, {"KB_F24", 313}, {"KB_F25", 314}, {"KB_KP_0", 320}, {"KB_KP_1", 321}, {"KB_KP_2", 322}, {"KB_KP_3", 323}, {"KB_KP_4", 324}, {"KB_KP_5", 325},
        {"KB_KP_6", 326}, {"KB_KP_7", 327}, {"KB_KP_8", 328}, {"KB_KP_9", 329}, {"KB_KP_DECIMAL", 330}, {"KB_KP_DIVIDE", 331}, {"KB_KP_MULTIPLY", 332},
        {"KB_KP_SUBTRACT", 333}, {"KB_KP_ADD", 334}, {"KB_KP_ENTER", 335}, {"KB_KP_EQUAL", 336}, {"KB_LEFT_SHIFT", 340}, {"KB_LEFT_CONTROL", 341}, {"KB_LEFT_ALT", 342},
        {"KB_LEFT_SUPER", 343}, {"KB_RIGHT_SHIFT", 344}, {"KB_RIGHT_CONTROL", 345}, {"KB_RIGHT_ALT", 346}, {"KB_RIGHT_SUPER", 347}, {"KB_MENU", 348},
        
        {"MOUSE_1", 0}, {"MOUSE_2", 1}, {"MOUSE_3", 2}, {"MOUSE_4", 3}, {"MOUSE_5", 4}, {"MOUSE_6", 5}, {"MOUSE_7", 6},
        {"MOUSE_8", 7}, {"MOUSE_LEFT", 0}, {"MOUSE_RIGHT", 1}, {"MOUSE_MIDDLE", 2}, {"MOUSE_X", 998}, {"MOUSE_Y", 999},
        {"MOUSE_SCROLL", 997}
    };
    return keys[key];
}

const std::string& getGlfwKeyName(int64_t key) {
    static std::map<int64_t, std::string> keys = {
        {-1, "KB_UNKNOWN"}, {32, "KB_SPACE"}, {39, "KB_APOSTROPHE"}, {44, "KB_COMMA"}, {45, "KB_MINUS"}, {46, "KB_PERIOD"}, {47, "KB_SLASH"}, {48, "KB_0"}, {49, "KB_1"},
        {50, "KB_2"}, {51, "KB_3"}, {52, "KB_4"}, {53, "KB_5"}, {54, "KB_6"}, {55, "KB_7"}, {56, "KB_8"}, {57, "KB_9"}, {59, "KB_SEMICOLON"}, {61, "KB_EQUAL"},
        {65, "KB_A"}, {66, "KB_B"}, {67, "KB_C"}, {68, "KB_D"}, {69, "KB_E"}, {70, "KB_F"}, {71, "KB_G"}, {72, "KB_H"}, {73, "KB_I"}, {74, "KB_J"}, {75, "KB_K"},
        {76, "KB_L"}, {77, "KB_M"}, {78, "KB_N"}, {79, "KB_O"}, {80, "KB_P"}, {81, "KB_Q"}, {82, "KB_R"}, {83, "KB_S"}, {84, "KB_T"}, {85, "KB_U"}, {86, "KB_V"},
        {87, "KB_W"}, {88, "KB_X"}, {89, "KB_Y"}, {90, "KB_Z"}, {91, "KB_LEFT_BRACKET"}, {92, "KB_BACKSLASH"}, {93, "KB_RIGHT_BRACKET"}, {96, "KB_GRAVE_ACCENT"},
        {161, "KB_WORLD_1"}, {162, "KB_WORLD_2"}, {256, "KB_ESCAPE"}, {257, "KB_ENTER"}, {258, "KB_TAB"}, {259, "KB_BACKSPACE"}, {260, "KB_INSERT"}, {261, "KB_DELETE"},
        {262, "KB_RIGHT"}, {263, "KB_LEFT"}, {264, "KB_DOWN"}, {265, "KB_UP"}, {266, "KB_PAGE_UP"}, {267, "KB_PAGE_DOWN"}, {268, "KB_HOME"}, {269, "KB_END"},
        {280, "KB_CAPS_LOCK"}, {281, "KB_SCROLL_LOCK"}, {282, "KB_NUM_LOCK"}, {283, "KB_PRINT_SCREEN"}, {284, "KB_PAUSE"}, {290, "KB_F1"}, {291, "KB_F2"}, {292, "KB_F3"},
        {293, "KB_F4"}, {294, "KB_F5"}, {295, "KB_F6"}, {296, "KB_F7"}, {297, "KB_F8"}, {298, "KB_F9"}, {299, "KB_F10"}, {300, "KB_F11"}, {301, "KB_F12"}, {302, "KB_F13"},
        {303, "KB_F14"}, {304, "KB_F15"}, {305, "KB_F16"}, {306, "KB_F17"}, {307, "KB_F18"}, {308, "KB_F19"}, {309, "KB_F20"}, {310, "KB_F21"}, {311, "KB_F22"},
        {312, "KB_F23"}, {313, "KB_F24"}, {314, "KB_F25"}, {320, "KB_KP_0"}, {321, "KB_KP_1"}, {322, "KB_KP_2"}, {323, "KB_KP_3"}, {324, "KB_KP_4"}, {325, "KB_KP_5"},
        {326, "KB_KP_6"}, {327, "KB_KP_7"}, {328, "KB_KP_8"}, {329, "KB_KP_9"}, {330, "KB_KP_DECIMAL"}, {331, "KB_KP_DIVIDE"}, {332, "KB_KP_MULTIPLY"},
        {333, "KB_KP_SUBTRACT"}, {334, "KB_KP_ADD"}, {335, "KB_KP_ENTER"}, {336, "KB_KP_EQUAL"}, {340, "KB_LEFT_SHIFT"}, {341, "KB_LEFT_CONTROL"}, {342, "KB_LEFT_ALT"},
        {343, "KB_LEFT_SUPER"}, {344, "KB_RIGHT_SHIFT"}, {345, "KB_RIGHT_CONTROL"}, {346, "KB_RIGHT_ALT"}, {347, "KB_RIGHT_SUPER"}, {348, "KB_MENU"},

        {3, "MOUSE_4"}, {4, "MOUSE_5"}, {5, "MOUSE_6"}, {6, "MOUSE_7"},
        {7, "MOUSE_8"}, {0, "MOUSE_LEFT"}, {1, "MOUSE_RIGHT"}, {2, "MOUSE_MIDDLE"}, {998, "MOUSE_X"}, {999, "MOUSE_Y"},
        {997, "MOUSE_SCROLL"}
    };

    return keys[key];
}

float& getMouseScrollAccum() {
    static float v = 0.0f;
    return v;
}

inline void onGlfwKey(GLFWwindow*, int, int, int, int);
inline void onGlfwMouseKey(GLFWwindow*, int, int, int);
inline void onGlfwMouseMove(GLFWwindow*, double, double);
inline void onGlfwMouseScroll(GLFWwindow*, double, double);
inline void onGlfwChar(GLFWwindow*, unsigned int);

inline void initGlfwInputCallbacks(GLFWwindow* window, InputMgr* input_mgr) {    
    glfwSetWindowUserPointer(window, (void*)input_mgr);
    glfwSetKeyCallback(window, &onGlfwKey);
    glfwSetMouseButtonCallback(window, &onGlfwMouseKey);
    glfwSetScrollCallback(window, &onGlfwMouseScroll);
    glfwSetCharCallback(window, onGlfwChar);
    //glfwSetCursorPosCallback(window, &onGlfwMouseMove);
}

inline void updateGlfwInput() {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    static double prev_pos[2] = { xpos, ypos };
    double diff[2] = { xpos - prev_pos[0], ypos - prev_pos[1] };
    InputMgr* ptr = (InputMgr*)glfwGetWindowUserPointer(window);

    ptr->set("MOUSE_X", (float)diff[0], false);
    ptr->set("MOUSE_Y", (float)diff[1], false);

    prev_pos[0] = xpos;
    prev_pos[1] = ypos;

    ptr->set("MOUSE_SCROLL", getMouseScrollAccum(), false);
    getMouseScrollAccum() = 0.0f;
}

inline void onGlfwKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto& io = ImGui::GetIO();
    io.KeysDown[key] = action == GLFW_PRESS;

    InputMgr* ptr = (InputMgr*)glfwGetWindowUserPointer(window);    
    if(action == GLFW_PRESS)
        ptr->set(getGlfwKeyName(key), 1.0f);
    else if(action == GLFW_RELEASE)
        ptr->set(getGlfwKeyName(key), 0.0f);
}

inline void onGlfwMouseKey(GLFWwindow* window, int button, int action, int mods)
{
    InputMgr* ptr = (InputMgr*)glfwGetWindowUserPointer(window);
    if(action == GLFW_PRESS)
        ptr->set(getGlfwKeyName(button), 1.0f);
    else if(action == GLFW_RELEASE)
        ptr->set(getGlfwKeyName(button), 0.0f);
}

inline void onGlfwMouseMove(GLFWwindow* window, double xpos, double ypos)
{
    static double prev_pos[2] = { xpos, ypos };
    double diff[2] = { xpos - prev_pos[0], ypos - prev_pos[1] };
    InputMgr* ptr = (InputMgr*)glfwGetWindowUserPointer(window);

    ptr->set("MOUSE_X", (float)diff[0]);
    ptr->set("MOUSE_Y", (float)diff[1]);

    prev_pos[0] = xpos;
    prev_pos[1] = ypos;    
}

inline void onGlfwMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
{
    InputMgr* ptr = (InputMgr*)glfwGetWindowUserPointer(window);
    getMouseScrollAccum() += (float)yoffset;
}

inline void onGlfwChar(GLFWwindow* window, unsigned int character) {
    auto& io = ImGui::GetIO();
    io.AddInputCharacter((ImWchar)character);
}

#endif
