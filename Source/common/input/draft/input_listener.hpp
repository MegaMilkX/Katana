#ifndef INPUT_LISTENER2_HPP
#define INPUT_LISTENER2_HPP


#include <map>
#include <string>
#include <functional>

enum InputActionEventType {
    INPUT_ACTION_PRESS,
    INPUT_ACTION_RELEASE,
    INPUT_ACTION_HOLD,
    INPUT_ACTION_TAP
};

struct InputActionDesc {
    InputActionEventType type;
    std::string          name;
};

class InputActionListener {
    std::vector<InputActionDesc> expected_actions;
public:
    void bindPressAction(const char* name, std::function<void(void)> cb) {
        expected_actions.push_back(InputActionDesc{ INPUT_ACTION_PRESS, name });
    }
    void bindReleaseAction(const char* name, std::function<void(void)> cb) {
        expected_actions.push_back(InputActionDesc{ INPUT_ACTION_RELEASE, name });
    }
    void bindHoldAction(const char* name, std::function<void(void)> cb) {

    }
    void bindTapAction(const char* name, std::function<void(void)> cb) {

    }

};


#endif
