#ifndef OCTREE_AABB_HPP
#define OCTREE_AABB_HPP

#include "../gfxm.hpp"

#include <vector>
#include <set>
#include <list>

#include "../debug_draw.hpp"

enum OCTREE_CELL_CHILD_ID {
    OCTREE_CELL_FRONT_LEFT_BOTTOM, // 000
    OCTREE_CELL_FRONT_RIGHT_BOTTOM, // 001
    OCTREE_CELL_FRONT_LEFT_TOP, // 010
    OCTREE_CELL_FRONT_RIGHT_TOP, // 011
    OCTREE_CELL_BACK_LEFT_BOTTOM, // 100
    OCTREE_CELL_BACK_RIGHT_BOTTOM, // 101
    OCTREE_CELL_BACK_LEFT_TOP, // 110
    OCTREE_CELL_BACK_RIGHT_TOP // 111
};

class OctreeObject;

class Octree {
public:
    struct Cell {
        int16_t depth;
        uint8_t child_index;
        int32_t parent;
        gfxm::aabb aabb;
        int32_t children[8];
        std::set<int32_t> objects;
    };

private:
    int16_t max_depth = 3;
    std::vector<Cell> cells;
    std::set<int32_t> free_cells;
    int32_t root_id;
    std::vector<OctreeObject> objects;
    std::set<int32_t> free_objects;
    std::set<int32_t> active_objects;

    int32_t acquireCell();
    int32_t acquireAsChildCell(uint8_t child_id, int32_t source_cell);
    void    freeCell(int32_t id);
    int32_t acquireObject();
    void    freeObject(int32_t id);

    void    expand(const gfxm::aabb& aabb);
    int32_t selectChild(int32_t cell, const gfxm::aabb& aabb);

    void    tryRemoveCell(int32_t cell);

public:
    Octree();
    int32_t fit(const gfxm::aabb& aabb);

    int32_t createObject(const gfxm::aabb& box, void* user_ptr = 0);
    void    deleteObject(int32_t uid);
    void    updateObject(int32_t uid, const gfxm::aabb& box);
    OctreeObject* getObject(int32_t uid);

    std::list<int32_t> listVisibleCells(const gfxm::frustum& fru);

    Cell*   getCell(int32_t id);

    void    debugDraw(DebugDraw& dd);

};

struct OctreeObject {
    int32_t cell;
    gfxm::aabb aabb;
    void* user_ptr;
};

#endif
