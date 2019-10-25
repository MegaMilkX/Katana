#ifndef ECS_ENTITY_HPP
#define ECS_ENTITY_HPP

#include <stdint.h>
#include <unordered_map>
#include <memory>

#include "attribute.hpp"

class ecsEntity {
    uint64_t attrib_bits;
    std::unordered_map<uint8_t, std::shared_ptr<ecsAttribBase>> attribs;
public:
    template<typename T>
    T* getAttrib() {
        T* ptr = 0;
        auto id = T::get_id_static();
        auto it = attribs.find(id);
        if(it == attribs.end()) {
            ptr = new T();
            attribs[id].reset(ptr);
        } else {
            ptr = (T*)it->second.get();
        }
        return ptr;
    }

    ecsAttribBase* getAttribPtr(uint8_t attrib_id) {
        auto it = attribs.find(attrib_id);
        if(it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }

    template<typename T>
    bool updateAttrib(const T& value) {
        auto it = attribs.find(T::get_id_static());
        if(it == attribs.end()) {
            return false;
        }
        *(T*)(it->second.get()) = (value);        
        return true;
    }

    const uint64_t& getAttribBits() const {
        return attrib_bits;
    }
    void setBit(attrib_id attrib) {
        attrib_bits |= (1 << attrib);
    }
    void clearBit(attrib_id attrib) {
        attrib_bits &= ~(1 << attrib);
    }
};

#endif
