#ifndef KT_ENTITY_HPP
#define KT_ENTITY_HPP

#include "component/component.hpp"
#include "../archetype/archetype.hpp"


class ktGameWorld;
class ktEntity {
    friend ktGameWorld;

    ktGameWorld*     world = 0;
    ktArchetype* archetype;
    int              arch_array_index = -1;

public:
    ktEntity(ktGameWorld* world);
    virtual ~ktEntity() {}

    ktGameWorld*        getWorld() { return world; }
    ktArchetype*    getArchetype() { return archetype; }

    template<typename COMPONENT_T>
    void enableComponent(COMPONENT_T* ptr) {
        world->enableComponent(this, ptr);
    }

    template<typename COMPONENT_T>
    void createComponent() {
        world->createComponent<COMPONENT_T>(this);
    }
    template<typename COMPONENT_T>
    COMPONENT_T* getComponent() {
        return (COMPONENT_T*)archetype->deref(arch_array_index, rttr::type::get<COMPONENT_T>());
    }
    template<typename COMPONENT_T>
    bool hasComponent() {
        return archetype->hasType(rttr::type::get<COMPONENT_T>());
    }
};


#endif
