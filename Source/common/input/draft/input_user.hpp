#ifndef INPUT_USER_HPP
#define INPUT_USER_HPP


#include <map>
#include <memory>

#include "adapters/input_adapter.hpp"


class InputUser {
    std::map<int, std::shared_ptr<InputAdapter>> adapters; 
public:

};


#endif
