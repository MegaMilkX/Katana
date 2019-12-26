#ifndef INPUT_LISTENER2_HPP
#define INPUT_LISTENER2_HPP


#include <map>
#include <string>
#include <functional>

enum InputActionEventType {
    INPUT_ACTION_PRESS,
    INPUT_ACTION_PRESS_REPEAT,
    INPUT_ACTION_RELEASE,
    INPUT_ACTION_TAP
};

typedef size_t input_action_uid_t;

struct InputActionEvent {
    InputActionEventType type;
    input_action_uid_t   action;
    int                  user_id;
};

class InputListenerBase {
public:
    virtual ~InputListenerBase() {}

    virtual bool onAction(InputActionEventType type, input_action_uid_t action_uid, int user_id) = 0;
};


#endif
