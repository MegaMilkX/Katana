#ifndef DOC_ECS_WORLD_GUI_HELPERS_HPP
#define DOC_ECS_WORLD_GUI_HELPERS_HPP

#include "state.hpp"


void saveTemplate(ecsEntityHandle hdl);

void imguiEntityListItemContextMenu(const char* string_id, ecsEntityHandle hdl, entity_id& selected_ent);

void imguiEntityListItem_(
    ecsEntityHandle hdl, 
    const std::string& name, 
    entity_id& selected_ent
);

void imguiEntityList_(
    ecsWorld* cur_world, 
    const std::vector<entity_id>& entities,
    entity_id& selected_ent
);


void imguiEntityAttributeList(DocEcsWorldState& state);


#endif
