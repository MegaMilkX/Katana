#ifndef INPUT_DEVICE_TPL_HPP
#define INPUT_DEVICE_TPL_HPP


#include "input_device.hpp"
#include "adapters/input_adapter.hpp"

template<typename ADAPTER_TYPE>
class InputDeviceTpl : public InputDevice {
    ADAPTER_TYPE* adapter = 0;
public:
    InputAdapter* getAdapter() override {
        return adapter;
    }
};


#endif
