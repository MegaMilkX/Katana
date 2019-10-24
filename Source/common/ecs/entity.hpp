#ifndef ECS_ENTITY_HPP
#define ECS_ENTITY_HPP

#include <stdint.h>
#include <unordered_map>
#include <memory>

#include "attribute.hpp"

typedef size_t entity_id;

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
