#ifndef INPUT_LISTENER_WRAP_HPP
#define INPUT_LISTENER_WRAP_HPP

#include "../common/input/input_mgr.hpp"

class InputListenerWrap {
public:
    InputListenerWrap() {
        input_lis = input().createListener();
    }
    virtual ~InputListenerWrap() {
        input().removeListener(input_lis);
    }

    void bindAxis(const std::string& name, InputListener::axis_cb_t cb) {
        input_lis->bindAxis(name, cb);
    }
    void bindActionPress(const std::string& action, InputListener::action_cb_t cb) {
        input_lis->bindActionPress(action, cb);
    }
    void bindActionRelease(const std::string& action, InputListener::action_cb_t cb) {
        input_lis->bindActionRelease(action, cb);
    }
private:
    InputListener* input_lis = 0;
};

#endif
