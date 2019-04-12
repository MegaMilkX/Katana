#ifndef INPUT_LISTENER_HPP
#define INPUT_LISTENER_HPP

#include <functional>
#include <map>
#include <string>

class InputListener {
public:
    typedef std::function<void(float)> axis_cb_t;
    typedef std::function<void(void)> action_cb_t;

    void bindAxis(const std::string& axis, axis_cb_t cb) {
        axis_callbacks[axis] = cb;
    }
    void bindActionPress(const std::string& action, action_cb_t cb) {
        press_callbacks[action] = cb;
    }
    void bindActionRelease(const std::string& action, action_cb_t cb) {
        release_callbacks[action] = cb;
    }

    void triggerAxis(const std::string& axis, float value) {
        if(axis_callbacks.count(axis)) {
            axis_callbacks[axis](value);
        }
    }
    void triggerPress(const std::string& action) {
        if(press_callbacks.count(action)) {
            press_callbacks[action]();
        }
    }
    void triggerRelease(const std::string& action) {
        if(release_callbacks.count(action)) {
            release_callbacks[action]();
        }
    }
private:
    std::map<std::string, axis_cb_t> axis_callbacks;
    std::map<std::string, action_cb_t> press_callbacks;
    std::map<std::string, action_cb_t> release_callbacks;
};

#endif
