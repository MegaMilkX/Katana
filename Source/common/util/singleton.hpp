#ifndef UTIL_SINGLETON_HPP
#define UTIL_SINGLETON_HPP

template<typename T>
class Singleton {
    static T* instance;
public:
    static T* get();
protected:
    Singleton() {}
private:
    Singleton(Singleton const &) = delete;
    Singleton& operator=(Singleton const &) = delete;
};

template<typename T>
T* Singleton<T>::instance = 0;
template<typename T>
T* Singleton<T>::get() {
    if(!instance) instance = new T();
    return instance;
}

#endif
