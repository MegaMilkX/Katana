#ifndef ECS_ENTITY_HPP
#define ECS_ENTITY_HPP

#include <stdint.h>

class ktEntity {
    uint64_t uid;
public:
    ktEntity(uint64_t uid)
    : uid(uid) {}
    bool operator<(const ktEntity& other) const {
        return uid < other.uid;
    }
    uint64_t getUid() const { 
        return uid; 
    }
};

#endif
