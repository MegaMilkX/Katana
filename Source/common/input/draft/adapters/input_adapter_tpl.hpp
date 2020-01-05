#ifndef INPUT_ADAPTER_TPL_HPP
#define INPUT_ADAPTER_TPL_HPP

#include <assert.h>
#include <stdint.h>
#include "input_adapter.hpp"

template<int KEY_COUNT, INPUT_ADAPTER_TYPE ADAPTER_TYPE>
class InputAdapterTpl : public InputAdapter {
    float keys[KEY_COUNT];
public:
    InputAdapterTpl() {
        clear();
    }

    INPUT_ADAPTER_TYPE getType() override {
        return ADAPTER_TYPE;
    }
    void clear() override {
        memset(keys, 0, sizeof(keys));
    }
    size_t keyCount() const override {
        return KEY_COUNT;
    }
    float getKeyState(size_t key) const override {
        return keys[key];
    }
    void setKeyState(size_t key, float value) override {
        assert(key < KEY_COUNT);
        if(keys[key] < value) {
            keys[key] = value;
        }
    }
};


#endif
