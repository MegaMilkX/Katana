#ifndef INPUT_ADAPTER_TPL_HPP
#define INPUT_ADAPTER_TPL_HPP

#include <assert.h>
#include <stdint.h>
#include "input_adapter.hpp"

template<int KEY_COUNT, int AXIS_COUNT>
class InputAdapterTpl : public InputAdapter {
    int16_t keys[KEY_COUNT];
    int16_t axes[AXIS_COUNT];
public:
    InputAdapterTpl() {
        clear();
    }

    input_adapter_type_id getTypeId() override {
        return 0; // TODO:
    }
    void clear() override {
        memset(keys, 0, sizeof(keys));
        memset(axes, 0, sizeof(axes));
    }
    size_t keyCount() const override {
        return KEY_COUNT;
    }
    size_t axisCount() const override {
        return AXIS_COUNT;
    }
    int16_t getKeyState(size_t key) const override {
        return keys[key];
    }
    int16_t getAxisState(size_t axis) const override {
        return axes[axis];
    }
    void setKeyState(size_t key, int16_t value) override {
        assert(key < KEY_COUNT);
        keys[key] = value;
    }
};


#endif
