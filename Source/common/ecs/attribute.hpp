#ifndef ECS_ATTRIBUTE_HPP
#define ECS_ATTRIBUTE_HPP

#include <stdint.h>

typedef int32_t attrib_id;

inline uint64_t next_attrib_id() {
    static uint64_t id = 0;
    return id++;
}

class ecsAttribBase {
public:
    virtual ~ecsAttribBase() {}
    virtual uint64_t get_id() const = 0;
};

template<typename T>
class ecsAttrib : public ecsAttribBase {
public:
    static uint64_t get_id_static() {
        static uint64_t id = next_attrib_id();
        return id;
    } 
    uint64_t get_id() const override {
        return get_id_static();
    }
};

template<typename T>
class ecsExclude {
public:
};

#endif
