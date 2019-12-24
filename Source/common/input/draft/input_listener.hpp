#ifndef INPUT_LISTENER2_HPP
#define INPUT_LISTENER2_HPP


#include <string>
#include <functional>

enum InputActionEventType {
    INPUT_ACTION_ON_PRESS,
    INPUT_ACTION_ON_RELEASE
};

struct InputActionListener {
    InputActionEventType      type = INPUT_ACTION_ON_PRESS;
    std::string               action_name;
    std::function<void(void)> callback;
};


#endif
