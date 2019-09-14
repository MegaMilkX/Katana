#ifndef RTTR_WRAP_HPP
#define RTTR_WRAP_HPP

#include <rttr/type>
#include <rttr/registration>

#include "../util/log.hpp"

template<typename BASE_T>
BASE_T* rttrCreate(const std::string& type_name) {
    rttr::type t = rttr::type::get_by_name(type_name);
    if(!t.is_valid()) {
        LOG_WARN("Reflection: " << type_name << " - invalid type");
        return 0;
    }
    rttr::variant var = t.create();
    if(!var.is_valid()) {
        LOG_WARN("Reflection: failed to create instance of type '" << type_name << "'");
        return 0;
    }
    if(!var.get_type().is_pointer()) {
        LOG_WARN("Reflection: invalid constructor policy for type '" << type_name << "'");
        return 0;
    }
    BASE_T* ptr = var.get_value<BASE_T*>();
    if(!ptr) {
        LOG_WARN("Reflection: ptr is null");
        return 0;
    }
    return ptr;
}

#endif
