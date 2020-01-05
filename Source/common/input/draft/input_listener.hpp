#ifndef INPUT_LISTENER2_HPP
#define INPUT_LISTENER2_HPP


#include <map>
#include <string>
#include <functional>

#include "input_types.hpp"

class InputListenerBase {
public:
    virtual ~InputListenerBase() {}

    virtual bool onAction(const InputActionEvent& evt) = 0;
    virtual bool onAxis(const InputAxisEvent& evt) = 0;
};


#endif
