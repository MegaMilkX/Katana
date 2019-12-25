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
    std::function<void(void)> callback;
};

struct InputActionListener {
    std::vector<InputActionDesc> expected_actions;
};


#endif
