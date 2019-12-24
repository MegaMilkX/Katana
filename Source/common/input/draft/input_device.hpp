#ifndef INPUT_DEVICE_2_HPP
#define INPUT_DEVICE_2_HPP


#include "adapters/input_adapter.hpp"
#include "input_user.hpp"

class InputDevice {
public:
    virtual void update() = 0;
    virtual void          setUser(InputUser* usr) = 0;
    virtual InputAdapter* getAdapter() = 0;
};


#endif
