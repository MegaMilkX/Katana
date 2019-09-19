#ifndef OCTREE_AABB_HPP
#define OCTREE_AABB_HPP

#include "../gfxm.hpp"

#include <vector>
#include <set>

#include "../debug_draw.hpp"

enum OCTREE_CELL_CHILD_ID {
    OCTREE_CELL_FRONT_LEFT_BOTTOM, // 000
    OCTREE_CELL_FRONT_RIGHT_BOTTOM, // 001
    OCTREE_CELL_FRONT_RIGHT_TOP, // 010
    OCTREE_CELL_FRONT_LEFT_TOP, // 011
    OCTREE_CELL_BACK_LEFT_BOTTOM, // 100
    OCTREE_CELL_BACK_RIGHT_BOTTOM, // 101
    OCTREE_CELL_BACK_RIGHT_TOP, // 110
    OCTREE_CELL_BACK_LEFT_TOP // 111
};

struct OctreeCell {
    gfxm::aabb aabb;
    int32_t children[8];
};

class Octree {
    std::vector<OctreeCell> cells;
    std::set<int32_t> free_cells;
    int32_t root_id;

    int32_t acquireCell();
    void    freeCell(int32_t id);

    void    expand(const gfxm::aabb& aabb);

public:
    Octree();
    int32_t fit(const gfxm::aabb& aabb);

    void    debugDraw(DebugDraw& dd);

};

#endif
