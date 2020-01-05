#ifndef INPUT_TYPES_HPP
#define INPUT_TYPES_HPP


#include <stdint.h>

enum InputActionEventType {
    INPUT_ACTION_PRESS,
    INPUT_ACTION_PRESS_REPEAT,
    INPUT_ACTION_RELEASE,
    INPUT_ACTION_TAP
};

typedef int32_t input_action_uid_t;
typedef int32_t input_axis_uid_t;

struct InputActionEvent {
    InputActionEventType type;
    input_action_uid_t   action;
    int                  user_id;
};

struct InputAxisEvent {
    input_axis_uid_t axis;
    float            value;
    int              user_id;
};


#endif
