#ifndef TYPE_MAP_HPP
#define TYPE_MAP_HPP

#include <rttr/type>
#include <map>
#include <memory>

template<typename BASE_T>
class type_map {
public:
    template<typename T>
    T* get() {
        
    }
private:
    std::map<rttr::type, std::shared_ptr<BASE_T>> map;
};

#endif
