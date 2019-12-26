#ifndef INPUT_ADAPTER_HPP
#define INPUT_ADAPTER_HPP


#include <stdint.h>

enum INPUT_ADAPTER_TYPE {
    INPUT_ADAPTER_KEYBOARD,
    INPUT_ADAPTER_MOUSE,
    INPUT_ADAPTER_GENERIC_GAMEPAD,
    INPUT_ADAPTER_XBOX,
    INPUT_ADAPTER_DUALSHOCK4,
    INPUT_ADAPTER_STEERING_WHEEL
};

class InputAdapter {
public:
    virtual ~InputAdapter() {}

    virtual INPUT_ADAPTER_TYPE getType() = 0;
    virtual void clear() = 0;
    virtual size_t keyCount() const = 0;
    virtual float getKeyState(size_t key) const = 0;
    virtual void setKeyState(size_t key, float value) = 0;
};


#endif
