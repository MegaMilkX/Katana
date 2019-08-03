#ifndef KT_PROPERTY_HPP
#define KT_PROPERTY_HPP

#include <memory>

template<typename T>
class ktProperty {
    std::shared_ptr<T> data;
public:
    ktProperty()
    : data(new T()) {}
    ktProperty(const ktProperty<T>& other) 
    : data(other.data) {}
    ktProperty(const T& value)
    : data(new T(value)) {}

    ktProperty& operator=(const ktProperty<T>& other) {
        data = other.data;
    }

    T& operator=(const T& value) {
        *data.get() = value;
    }

    operator T() const {
        return *data.get();
    }
    operator T() {
        return *data.get();
    }
    operator T&() {
        return *data.get();
    }
    operator const T&() const {
        return *data.get();
    }
};

#endif
