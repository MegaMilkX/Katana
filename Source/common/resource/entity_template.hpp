#ifndef ENTITY_TEMPLATE_HPP
#define ENTITY_TEMPLATE_HPP


#include <set>
#include "resource.h"
#include "../ecs/entity_handle.hpp"


class EntityTemplate : public Resource, public std::enable_shared_from_this<EntityTemplate> {
    RTTR_ENABLE(Resource)

    ecsEntityHandle hdl;
    std::set<ecsWorld*> referencing_worlds;

public:
    EntityTemplate();

    ecsEntityHandle getEntity();

    void insertReferencingWorld(ecsWorld* wrld);
    void eraseReferencingWorld(ecsWorld* wrld);

    void updateDerivedEntities();
    void update(ecsEntityHandle hdl);

    void serialize(out_stream& out) override;

    bool deserialize(in_stream& in, size_t sz) override;

    const char* getWriteExtension() const { return "entity"; }

};


#endif
