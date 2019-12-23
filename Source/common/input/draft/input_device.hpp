#ifndef INPUT_DEVICE_2_HPP
#define INPUT_DEVICE_2_HPP


#include "adapters/input_adapter.hpp"

class InputDevice {
public:
    virtual void update() = 0;
    virtual InputAdapter* getAdapter() = 0;
};


#endif
