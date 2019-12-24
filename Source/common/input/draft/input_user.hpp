#ifndef INPUT_USER_HPP
#define INPUT_USER_HPP


#include <map>
#include <memory>

#include <rttr/type>

#include "adapters/input_adapter.hpp"


class InputUser {
    std::map<rttr::type, std::shared_ptr<InputAdapter>> adapters; 
public:
    template<typename ADAPTER_T>
    ADAPTER_T* getAdapter() {
        rttr::type t = rttr::type::get<ADAPTER_T>();
        auto it = adapters.find(t);
        if(it == adapters.end()) {
            it = adapters.insert(adapters.begin(), std::make_pair(t, std::shared_ptr<InputAdapter>(new ADAPTER_T)));
        }
        return (ADAPTER_T*)it->second.get();
    }
};


#endif
