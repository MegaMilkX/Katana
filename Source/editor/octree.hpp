#ifndef OCTREE_HPP
#define OCTREE_HPP

#include <vector>
#include <set>
#include "../common/debug_draw.hpp"

enum OCTREE_LEAF_INDEX {
    OCT_BACK_TOP_LEFT,
    OCT_BACK_TOP_RIGHT,
    OCT_FRONT_TOP_RIGHT,
    OCT_FRONT_TOP_LEFT,
    OCT_BACK_BOTTOM_LEFT,
    OCT_BACK_BOTTOM_RIGHT,
    OCT_FRONT_BOTTOM_RIGHT,
    OCT_FRONT_BOTTOM_LEFT
};

struct OctreeObject;
struct OctreeCell {
    gfxm::aabb aabb;
    bool is_leaf = true;
    int depth;
    OCTREE_LEAF_INDEX leaf_index = OCT_BACK_TOP_LEFT;
    size_t parent = 0;
    size_t child_indices[8];
    std::set<OctreeObject*> objects;
    size_t object_count = 0;
};

struct OctreeObject {
    gfxm::vec3 pos;
    gfxm::aabb box = gfxm::aabb(gfxm::vec3(-.5f,-.5f,-.5f), gfxm::vec3(.5f,.5f,.5f));
    int64_t cell_id = -1;
};

class Octree {
public:
    Octree(float base_size = 20.0f, int max_depth = 5) {
        leaves.insert(cells.size());
        cells.emplace_back(OctreeCell{
            gfxm::aabb(
                gfxm::vec3(-base_size*0.5f,-base_size*0.5f,-base_size*0.5f), 
                gfxm::vec3(base_size*0.5f,base_size*0.5f,base_size*0.5f)
            ),
            true,
            max_depth
        });
    }

    OctreeObject* createObject() {
        return new OctreeObject();
    }
    void destroyObject(OctreeObject* o) {
        delete o;
    }

    void expand() {
        size_t id = _acquireCell();
        gfxm::aabb root_aabb = cells[root_id].aabb;
        float side = root_aabb.to.x - root_aabb.from.x;
        gfxm::vec3 center(root_aabb.to.x, root_aabb.from.y, root_aabb.to.z);
        cells[id].aabb = gfxm::aabb(
            gfxm::vec3(root_aabb.from.x, root_aabb.from.y - side, root_aabb.from.z),
            gfxm::vec3(root_aabb.to.x + side, root_aabb.to.y, root_aabb.to.z + side)
        );
        cells[id].child_indices[0] = root_id;
        cells[id].depth = cells[root_id].depth++;
        cells[id].is_leaf = false;
        cells[id].object_count = cells[root_id].object_count;
        cells[id].parent = id;
        auto caabb = cells[id].aabb;
        cells[root_id].parent = id;
        root_id = id;

        _acquireChild(id, 1, cells[id].aabb);
        _acquireChild(id, 2, cells[id].aabb); 
        _acquireChild(id, 3, cells[id].aabb);
        _acquireChild(id, 4, cells[id].aabb);
        _acquireChild(id, 5, cells[id].aabb);
        _acquireChild(id, 6, cells[id].aabb);
        _acquireChild(id, 7, cells[id].aabb);
    }

    void fit(OctreeObject* o) {
        if(o->cell_id >= 0) {
            _removeObject(o->cell_id, o);
        }
        fit(root_id, o);
        _cleanupEmptyCells(root_id);
    }
    bool fit(size_t cell_id, OctreeObject* o) {
        if(gfxm::aabb_in_aabb(gfxm::aabb(o->box.from + o->pos, o->box.to + o->pos), cells[cell_id].aabb)) {
            if(cells[cell_id].is_leaf) {
                if(cells[cell_id].depth == 0) {
                    o->cell_id = cell_id;
                    _addObject(cell_id, o);
                    return true;
                } else {
                    split(cell_id);
                    for(size_t i = 0; i < 8; ++i) {
                        if(fit(cells[cell_id].child_indices[i], o)) {
                            return true;
                        }
                    }
                }
            } else {
                for(size_t i = 0; i < 8; ++i) {
                    if(fit(cells[cell_id].child_indices[i], o)) {
                        return true;
                    }
                }
            }

            o->cell_id = cell_id;
            _addObject(cell_id, o);
            return true;
        }
        
        return false;
    }

    void fit(const gfxm::vec3& pt) {
        fit(root_id, pt);
    }
    bool fit(size_t cell_id, const gfxm::vec3& pt) {
        if(gfxm::point_in_aabb(cells[cell_id].aabb, pt)) {
            if(cells[cell_id].is_leaf) {
                if(cells[cell_id].depth == 0) {
                    return true;
                } else {
                    split(cell_id);
                    for(size_t i = 0; i < 8; ++i) {
                        if(fit(cells[cell_id].child_indices[i], pt)) {
                            return true;
                        }
                    }
                }
            } else {
                for(size_t i = 0; i < 8; ++i) {
                    if(fit(cells[cell_id].child_indices[i], pt)) {
                        return true;
                    }
                }
            }
        }
        
        return false;
    }

    void subdivide() {
        size_t initial_cell_count = cells.size();
        for(size_t i = 0; i < initial_cell_count; ++i) {
            split(i);
        }
    }

    void split(size_t id) {
        if(id>=cells.size()) return;
        cells.reserve(cells.size() + 8);
        OctreeCell& c = cells[id];
        if(!c.is_leaf) return;
        if(c.depth == 0) return;
        leaves.erase(id);
        c.is_leaf = false;
        gfxm::vec3 center = c.aabb.from + (c.aabb.to - c.aabb.from) * 0.5f;
        
        size_t cid = 0;

        _acquireChild(id, 0, cells[id].aabb);
        _acquireChild(id, 1, cells[id].aabb);
        _acquireChild(id, 2, cells[id].aabb);
        _acquireChild(id, 3, cells[id].aabb);
        _acquireChild(id, 4, cells[id].aabb);
        _acquireChild(id, 5, cells[id].aabb);
        _acquireChild(id, 6, cells[id].aabb);
        _acquireChild(id, 7, cells[id].aabb);
    }

    void debugDraw(DebugDraw* dd) {
        for(auto l : leaves) {
            dd->aabb(
                cells[l].aabb,
                gfxm::vec3(0.5f,0,0.25f)
            );            
        }
    }
private:
    size_t root_id = 0;
    std::set<size_t> leaves;
    std::vector<OctreeCell> cells;
    std::set<size_t> free_cell_indices;

    size_t _acquireCell() {
        size_t cid = 0;
        if(free_cell_indices.empty()) {
            cid = cells.size();
            cells.emplace_back(OctreeCell());
        } else {
            cid = *free_cell_indices.begin();
            free_cell_indices.erase(cid);
        }
        return cid;
    }
    void _freeCell(size_t cell) {
        free_cell_indices.insert(cell);
        if(cells[cell].is_leaf) {
            leaves.erase(cell);
        }
    }

    void _acquireChild(size_t pid, int slot, const gfxm::aabb& paabb) {
        gfxm::vec3 center = paabb.from + (paabb.to - paabb.from) * 0.5f;
        float vdat[] = {
            paabb.from.x, paabb.from.y, paabb.from.z,
            center.x, center.y, center.z,
            paabb.to.x, paabb.to.y, paabb.to.z
        };
        static std::vector<std::vector<int>> indices = {
            { 0, 4, 2, 3, 7, 5 },
            { 3, 4, 2, 6, 7, 5 },
            { 3, 4, 5, 6, 7, 8 },
            { 0, 4, 5, 3, 7, 8 },
            { 0, 1, 2, 3, 4, 5 },
            { 3, 1, 2, 6, 4, 5 },
            { 3, 1, 5, 6, 4, 8 },
            { 0, 1, 5, 3, 4, 8 }
        };

        size_t cid = _acquireCell();
        cells[pid].child_indices[slot] = cid;
        leaves.insert(cid);

        gfxm::aabb box(
            gfxm::vec3(vdat[indices[slot][0]], vdat[indices[slot][1]], vdat[indices[slot][2]]), 
            gfxm::vec3(vdat[indices[slot][3]], vdat[indices[slot][4]], vdat[indices[slot][5]])
        );

        cells[cid] = OctreeCell{
            box, true, cells[pid].depth - 1, (OCTREE_LEAF_INDEX)slot, pid
        };
    }

    void _addObject(size_t cell, OctreeObject* o) {
        cells[cell].objects.insert(o);
        _incrementObjectCountRecursive(cell);
    }
    void _removeObject(size_t cell, OctreeObject* o) {
        cells[cell].objects.erase(o);
        _decrementObjectCountRecursive(cell);
        o->cell_id = -1;
    }
    void _incrementObjectCountRecursive(size_t cell) {
        cells[cell].object_count++;
        if(cells[cell].parent != cell) {
            _incrementObjectCountRecursive(cells[cell].parent);
        }
    }
    void _decrementObjectCountRecursive(size_t cell) {
        cells[cell].object_count--;
        if(cells[cell].parent != cell) {
            _decrementObjectCountRecursive(cells[cell].parent);
        }
    }
    void _cleanupEmptyCells(size_t cell) {
        if(cells[cell].object_count == 0) {
            _destroyChildren(cell);
        } else if(!cells[cell].is_leaf) {
            for(size_t i = 0; i < 8; ++i) {
                _cleanupEmptyCells(cells[cell].child_indices[i]);
            }
        } 
    }
    void _destroyChildren(size_t cell) {
        if(!cells[cell].is_leaf) {
            for(size_t i = 0; i < 8; ++i) {
                _destroyCell(cells[cell].child_indices[i]);
            }
        }
        cells[cell].is_leaf = true;
        leaves.insert(cell);
    }
    void _destroyCell(size_t cell) {
        if(!cells[cell].is_leaf) {
            for(size_t i = 0; i < 8; ++i) {
                _destroyCell(cells[cell].child_indices[i]);
            }
        }
        _freeCell(cell);
    }
};

#endif
