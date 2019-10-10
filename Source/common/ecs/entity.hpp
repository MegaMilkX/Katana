#ifndef ECS_ENTITY_HPP
#define ECS_ENTITY_HPP

#include <stdint.h>

class ktEcsWorld;
class ktEntity {
    ktEcsWorld* world;
    uint64_t    uid;
public:
    ktEntity(ktEcsWorld* world, uint64_t uid)
    : world(world), uid(uid) {}
    bool operator<(const ktEntity& other) const { return uid < other.uid; }
    uint64_t getUid() const { return uid; }

    
};

#endif
