#ifndef INPUT_DEVICE_KB_MOUSE_WIN32_HPP
#define INPUT_DEVICE_KB_MOUSE_WIN32_HPP

#include <glfw/glfw3.h>

#include "input.hpp"

class InputDeviceGlfwKbMouse : public InputDevice {
public:
    InputDeviceGlfwKbMouse(GLFWwindow* window)
    : window(window) {
        glfwSetWindowUserPointer(window, (void*)this);

        glfwSetKeyCallback(window, &InputDeviceGlfwKbMouse::_onKey);
        glfwSetMouseButtonCallback(window, &InputDeviceGlfwKbMouse::_onMouseKey);
        glfwSetScrollCallback(window, &InputDeviceGlfwKbMouse::_onMouseScroll);
    
        keys = {
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
    }
    ~InputDeviceGlfwKbMouse() {
        glfwSetWindowUserPointer(window, 0);
    }
    
    virtual void init(InputMgr* mgr) {
        this->mgr = mgr;
        
    }
    virtual InputNode* getInputNode(const std::string& name) {
        auto it = keys.find(name);
        if(it != keys.end()) {
            auto node_it = nodes.find(it->second);
            if(node_it == nodes.end()) {
                nodes[it->second].reset(new InputNode());
            }
        }
        return nodes[it->second].get();
    }
    virtual void update() {

    }
private:
    GLFWwindow* window;
    InputMgr* mgr;
    std::map<std::string, int> keys;
    std::map<int, std::shared_ptr<InputNode>> nodes;

    static void _onKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        InputDeviceGlfwKbMouse* ptr = (InputDeviceGlfwKbMouse*)glfwGetWindowUserPointer(window);
        auto it = ptr->nodes.find(key);
        if(it == ptr->nodes.end()) return;

        if(action == GLFW_PRESS)
            it->second->set(std::numeric_limits<int16_t>().max());
        else if(action == GLFW_RELEASE)
            it->second->set(0);
    }

    static void _onMouseKey(GLFWwindow* window, int button, int action, int mods)
    {
        InputDeviceGlfwKbMouse* ptr = (InputDeviceGlfwKbMouse*)glfwGetWindowUserPointer(window);
        auto it = ptr->nodes.find(button);
        if(it == ptr->nodes.end()) return;

        if(action == GLFW_PRESS)
            it->second->set(std::numeric_limits<int16_t>().max());
        else if(action == GLFW_RELEASE)
            it->second->set(0);
    }

    static void _onMouseMove(GLFWwindow* window, double xpos, double ypos)
    {
        InputDeviceGlfwKbMouse* ptr = (InputDeviceGlfwKbMouse*)glfwGetWindowUserPointer(window);
        /*
        gfxm::vec2 mouse_pos;
        mouse_pos.x = (float)xpos;
        mouse_pos.y = (float)ypos;
        gfxm::vec2 mouse_pos_rel = mouse_pos - ptr->mouse_pos_prev;
        // X
        ptr->mouse_objects[ptr->keys_mouse["MOUSE_X"]]->Set(mouse_pos_rel.x);
        // Y
        ptr->mouse_objects[ptr->keys_mouse["MOUSE_Y"]]->Set(mouse_pos_rel.y); 

        ptr->mouse_pos_prev = mouse_pos;*/        
    }

    static void _onMouseScroll(GLFWwindow* window, double xoffset, double yoffset)
    {
        InputDeviceGlfwKbMouse* ptr = (InputDeviceGlfwKbMouse*)glfwGetWindowUserPointer(window);
        /*ptr->mouseScrollAccum += (float)yoffset;*/
    }
};

#endif