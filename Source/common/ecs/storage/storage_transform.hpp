#ifndef ECS_STORAGE_TRANSFORM_HPP
#define ECS_STORAGE_TRANSFORM_HPP


#include "world_storage.hpp"
#include "../../util/log.hpp"

#include "../tuples/tuple_transform.hpp"


class ecsCacheTransform : public ecsWorldStorage {
    std::vector<entity_id>  dirty_vec;
    size_t                  first_dirty_index = 0;

public:
    void onUpdateEnd() {
        // TODO: Clear dirty cache
    }

};


#endif
