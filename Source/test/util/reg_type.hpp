#ifndef REG_TYPE_HPP
#define REG_TYPE_HPP

#include <new>

#include <map>
#include <vector>
#include <memory>
#include <rttr/type>
#include <rttr/registration>
#include "../../common/util/static_run.h"

#include "handle_mgr.hpp"

template<typename T>
void constructor_impl(void* ptr) {
    new(ptr)T();
}
template<typename T>
void destructor_impl(void* ptr) {
    ((T*)ptr)->~T();
}
template<typename T>
std::shared_ptr<handle_mgr_base> construct_handle_mgr_impl() {
    return std::shared_ptr<handle_mgr_base>(new handle_mgr<T>());
}

typedef void(*constructor_impl_t)(void*);
typedef void(*destructor_impl_t)(void*);
typedef std::shared_ptr<handle_mgr_base>(*handle_mgr_constructor_impl_t)(void);
typedef handle_mgr_base&(*get_handle_mgr_t)(void);

void reg_in_place_construction(rttr::type t, constructor_impl_t constructor, destructor_impl_t destructor);
template<typename T>
void reg_in_place_construction(){
    reg_in_place_construction(
        rttr::type::get<T>(),
        &constructor_impl<T>,
        &destructor_impl<T>
    );
    
}

bool construct_at(rttr::type t, void* ptr);
template<typename T>
void construct_at(void* ptr) {
    construct_at(rttr::type::get<T>(), ptr);
}

bool destruct_at(rttr::type t, void* ptr);
template<typename T>
void destruct_at(void* ptr) {
    destruct_at(rttr::type::get<T>(), ptr);
}

handle_mgr_base* get_handle_mgr(rttr::type t);
void reg_handle_mgr_getter(rttr::type t, get_handle_mgr_t func);
template<typename T>
void reg_handle_mgr_getter() {
    reg_handle_mgr_getter(rttr::type::get<T>(), &get_handle_mgr_impl<T>);
}

template<typename T>
class reg_type : public rttr::registration::class_<T> {
public:
    reg_type(const std::string& tname)
    : rttr::registration::class_<T>(tname) {
        reg_in_place_construction<T>();
        reg_handle_mgr_getter<T>();

        rttr::registration::class_<T>(tname)
            .constructor<>()(
                rttr::policy::ctor::as_raw_ptr
            );
    }
};

template<typename T>
class ghandle {
public:
    ghandle()
    : ghandle(0) {}
    ghandle(handle_ h) 
    : hdl(h) {}

    ghandle& operator=(const handle_& h) {
        hdl = h;
    }
    operator handle_() const {
        return hdl;
    }
    operator bool() const {
        return get_handle_mgr<T>().deref(hdl) != 0;
    }
    T* deref() {
        return get_handle_mgr<T>().deref(hdl);
    }
    const T* deref() const {
        return get_handle_mgr<T>().deref(hdl);
    }
    T* operator->() {
        return deref();
    }
    const T* operator->() const {
        return deref();
    }
private:
    handle_ hdl;
};

#endif
