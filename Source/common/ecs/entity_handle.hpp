#ifndef ECS_ENTITY_HANDLE_HPP
#define ECS_ENTITY_HANDLE_HPP

#include "types.hpp"

class ecsWorld;
class ecsEntityHandle {
    entity_id id;
    ecsWorld* world = 0;
public:
    ecsEntityHandle() {}
    ecsEntityHandle(ecsWorld* world, entity_id ent)
    : world(world), id(ent) {}

    bool        isValid() const;

    entity_id   getId() const;
    ecsWorld*   getWorld();

    void        remove();

    // Get existing attribute
    template<typename T>
    T*          findAttrib() const;
    // Get attribute or create one if it doesn't exist
    template<typename T>
    T*          getAttrib();
    // Set attribute's value. Create it from that value if it doesn't exist
    template<typename T>
    void        setAttrib(const T& value);
    // Update attribute's value and send update notification to systems. Do nothing if attrib doesn't exist
    template<typename T>
    void        updateAttrib(const T& value);
    // Send an update notification to systems as if component T was changed
    template<typename T>
    void        signalUpdate();
    // Get attribute bitmask
    uint64_t    getAttribBitmask();
    // Get bitmask of attributes that are considered inherited from a template
    uint64_t    getInheritedAttribBitmask();

    operator bool() const {
        return isValid();
    }
};


template<typename T>
T* ecsEntityHandle::findAttrib() const {
    return world->findAttrib<T>(id);
}
template<typename T>
T* ecsEntityHandle::getAttrib() {
    return world->getAttrib<T>(id);
}
template<typename T>
void ecsEntityHandle::setAttrib(const T& value) {
    assert(false);
}
template<typename T>
void ecsEntityHandle::updateAttrib(const T& value) {
    assert(false);
}
template<typename T>
void ecsEntityHandle::signalUpdate() {
    world->signalAttribUpdate<T>(id);
}


#endif
