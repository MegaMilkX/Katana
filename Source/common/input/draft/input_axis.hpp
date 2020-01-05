#ifndef INPUT_AXIS2_HPP
#define INPUT_AXIS2_HPP


#include <vector>
#include <rttr/type>


struct InputAxisKey {
    rttr::type adapter_type;
    int        key;
    float      multiplier = 1.0f;
};

struct InputAxis {
    std::string name;
    std::vector<InputAxisKey> keys;
};


#endif
