#include "reg_type.hpp"

struct TypeDataStorage {
    std::map<rttr::type, constructor_impl_t> constructors;
    std::map<rttr::type, destructor_impl_t> destructors;
    std::map<rttr::type, get_handle_mgr_t> handle_mgr_getters;
};

static TypeDataStorage& get_type_data_storage() {
    static TypeDataStorage strg;
    return strg;
}

void reg_in_place_construction(
    rttr::type t, 
    constructor_impl_t constructor, 
    destructor_impl_t destructor
) {
    get_type_data_storage().constructors[t] = constructor;
    get_type_data_storage().destructors[t] = destructor;
}

bool construct_at(rttr::type t, void* ptr) {
    if(get_type_data_storage().constructors.count(t) == 0) return false;
    get_type_data_storage().constructors[t](ptr);
    return true;
}
bool destruct_at(rttr::type t, void* ptr) {
    if(get_type_data_storage().destructors.count(t) == 0) return false;
    get_type_data_storage().destructors[t](ptr);
    return true;
}

handle_mgr_base* get_handle_mgr(rttr::type t) {
    if(get_type_data_storage().handle_mgr_getters.count(t) == 0) return 0;
    else return &get_type_data_storage().handle_mgr_getters[t]();
}
void reg_handle_mgr_getter(rttr::type t, get_handle_mgr_t func) {
    get_type_data_storage().handle_mgr_getters[t] = func;
}