#ifndef INPUT_ACTION2_HPP
#define INPUT_ACTION2_HPP


#include <stdint.h>
#include <vector>

#include <rttr/type>

struct ButtonCombo {
    ButtonCombo(rttr::type adapter_type)
    : ButtonCombo(adapter_type, -1) {}
    ButtonCombo(rttr::type adapter_type, int key0, int key1 = -1, int key2 = -1)
    : adapter_type(adapter_type) {
        keys[0] = key0;
        keys[1] = key1;
        keys[2] = key2;
    }
    rttr::type  adapter_type;
    int         keys[3];
};

struct InputAction2 {
    std::string name;
    std::vector<ButtonCombo> inputs;
};


#endif
