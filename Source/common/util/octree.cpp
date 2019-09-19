#include "octree.hpp"

size_t Octree::acquireCell() {
    size_t cid = 0;
    if(free_cells.empty()) {
        cid = cells.size();
        cells.emplace_back(OctreeCell());
    } else {
        cid = *free_cells.begin();
        free_cells.erase(free_cells.begin());
    }
    return cid;
}
void Octree::freeCell(size_t id) {
    free_cells.insert(id);
}
