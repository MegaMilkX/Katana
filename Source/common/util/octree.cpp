#include "octree.hpp"

int32_t Octree::acquireCell() {
    int32_t cid = 0;
    if(free_cells.empty()) {
        cid = cells.size();
        cells.emplace_back(OctreeCell());
    } else {
        cid = *free_cells.begin();
        free_cells.erase(free_cells.begin());
    }
    cells[cid].children[0] = -1;
    cells[cid].children[1] = -1;
    cells[cid].children[2] = -1;
    cells[cid].children[3] = -1;
    cells[cid].children[4] = -1;
    cells[cid].children[5] = -1;
    cells[cid].children[6] = -1;
    cells[cid].children[7] = -1;
    return cid;
}
void Octree::freeCell(int32_t id) {
    free_cells.insert(id);
}

void Octree::expand(const gfxm::aabb& aabb) {
    OctreeCell& root = cells[root_id];
    const gfxm::vec3 root_center = (root.aabb.from + root.aabb.to) * .5f;
    const gfxm::vec3 target_center = (aabb.from + aabb.to) * .5f;

    int8_t root_child_id = 0;
    if(root_center.x > target_center.x) root_child_id |= 1;
    if(root_center.y > target_center.y) root_child_id |= 2;
    if(root_center.z > target_center.z) root_child_id |= 4;

    int32_t new_root_id = acquireCell();
    root = cells[root_id]; // root reference gets invalidated after acquireCell()
    OctreeCell& new_root = cells[new_root_id];
    new_root.children[root_child_id] = root_id;

    const gfxm::vec3 extents(root.aabb.to.x - root.aabb.from.x, root.aabb.to.y - root.aabb.from.y, root.aabb.to.z - root.aabb.from.z);
    const gfxm::vec3 id_mods(
      (root_child_id & 1) - .5f,
      ((root_child_id & 2) >> 1) - .5f,
      ((root_child_id & 4) >> 2) - .5f
    );

    new_root.aabb = gfxm::aabb(
        gfxm::vec3(
            root.aabb.from.x - (id_mods.x + .5f) * extents.x,
            root.aabb.from.y - (id_mods.y + .5f) * extents.y,
            root.aabb.from.z - (id_mods.z + .5f) * extents.z
        ),
        gfxm::vec3(
            root.aabb.to.x - (id_mods.x - .5f) * extents.x,
            root.aabb.to.y - (id_mods.y - .5f) * extents.y,
            root.aabb.to.z - (id_mods.z - .5f) * extents.z
        )  
    );
    root_id = new_root_id;    
}


Octree::Octree()
: root_id(0) {
    root_id = acquireCell();
    cells[root_id].aabb = gfxm::aabb(
        gfxm::vec3(-10, -10, -10),
        gfxm::vec3(10, 10, 10)
    );
}

int32_t Octree::fit(const gfxm::aabb& aabb) {
    while(!gfxm::aabb_in_aabb(aabb, cells[root_id].aabb)) {
        expand(aabb);
    }



    return 0;
}


void Octree::debugDraw(DebugDraw& dd) {
    for(auto& cell : cells) {
        dd.aabb(cell.aabb, gfxm::vec3(1.0f, 0.7f, 0.0f));
    }
}