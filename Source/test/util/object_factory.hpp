#ifndef OBJECT_FACTORY_HPP
#define OBJECT_FACTORY_HPP

#include "reg_type.hpp"

template<typename T>
class h_object {
public:
    h_object();
    h_object(handle_);

    h_object& operator=(const handle_& other);
    operator handle_() const;
    operator bool() const;
    T* operator->();
    const T* operator->() const;
private:
    handle_ hdl;
};

template<typename T>
class ObjectFactory {
public:
    h_object<T>         acquire();
};

#endif
