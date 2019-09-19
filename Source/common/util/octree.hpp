#ifndef OCTREE_AABB_HPP
#define OCTREE_AABB_HPP

#include "../gfxm.hpp"

#include <vector>
#include <set>

struct OctreeCell {
    gfxm::aabb aabb;
};

class Octree {
    std::vector<OctreeCell> cells;
    std::set<size_t> free_cells;

    size_t acquireCell();
    void freeCell(size_t id);

public:

};

#endif
