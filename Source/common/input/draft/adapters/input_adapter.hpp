#ifndef INPUT_ADAPTER_HPP
#define INPUT_ADAPTER_HPP


#include <stdint.h>

class InputAdapter {
public:
    virtual ~InputAdapter() {}

    virtual void clear() = 0;
    virtual size_t keyCount() const = 0;
    virtual size_t axisCount() const = 0;
    virtual int16_t getKeyState(size_t key) const = 0;
    virtual int16_t getAxisState(size_t axis) const = 0;
    virtual void setKeyState(size_t key, int16_t value) = 0;
};


#endif