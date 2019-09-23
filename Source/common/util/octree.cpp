#include "octree.hpp"

int32_t Octree::acquireCell() {
    int32_t cid = 0;
    if(free_cells.empty()) {
        cid = cells.size();
        cells.emplace_back(Cell());
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
    cells[cid].child_index = 0;
    cells[cid].parent = -1;
    cells[cid].depth = 0;
    return cid;
}
int32_t Octree::acquireAsChildCell(uint8_t child_id, int32_t source_cell) {
    int32_t cell = acquireCell();
    cells[cell].parent = source_cell;
    cells[cell].depth = cells[source_cell].depth + 1;
    cells[cell].child_index = child_id;
    cells[source_cell].children[child_id] = cell;

    const gfxm::aabb& source_box = cells[source_cell].aabb;
    gfxm::vec3 src_center = (source_box.from + source_box.to) * .5f;

    const float vertices[] = {
        source_box.from.x, source_box.from.y, source_box.from.z,
        src_center.x, src_center.y, src_center.z,
        source_box.to.x, source_box.to.y, source_box.to.z
    };
    static const std::vector<std::vector<int>> indices = {
        { 0, 1, 2, 3, 4, 5 },
        { 3, 1, 2, 6, 4, 5 },
        { 0, 4, 2, 3, 7, 5 },
        { 3, 4, 2, 6, 7, 5 },
        { 0, 1, 5, 3, 4, 8 },
        { 3, 1, 5, 6, 4, 8 },
        { 0, 4, 5, 3, 7, 8 },
        { 3, 4, 5, 6, 7, 8 }
    };

    cells[cell].aabb = gfxm::aabb(
        gfxm::vec3(
            vertices[indices[child_id][0]], 
            vertices[indices[child_id][1]],
            vertices[indices[child_id][2]]
        ),
        gfxm::vec3(
            vertices[indices[child_id][3]], 
            vertices[indices[child_id][4]],
            vertices[indices[child_id][5]]
        )
    );
    return cell;
}
void Octree::freeCell(int32_t id) {
    free_cells.insert(id);
    if(cells[id].parent >= 0) {
        cells[cells[id].parent].children[cells[id].child_index] = -1;
    }
}

int32_t Octree::acquireObject() {
    int32_t id = 0;
    if(free_objects.empty()) {
        id = objects.size();
        objects.emplace_back(OctreeObject());
    } else {
        id = *free_objects.begin();
        free_objects.erase(free_objects.begin());
    }
    active_objects.insert(id);
    return id;
}
void    Octree::freeObject(int32_t id) {
    free_objects.insert(id);
    active_objects.erase(id);
}

void Octree::expand(const gfxm::aabb& aabb) {
    const gfxm::vec3 root_center = (cells[root_id].aabb.from + cells[root_id].aabb.to) * .5f;
    const gfxm::vec3 target_center = (aabb.from + aabb.to) * .5f;

    int8_t root_child_id = 0;
    if(root_center.x > target_center.x) root_child_id |= 1;
    if(root_center.y > target_center.y) root_child_id |= 2;
    if(root_center.z > target_center.z) root_child_id |= 4;

    int32_t new_root_id = acquireCell();
    Cell& new_root = cells[new_root_id];
    new_root.children[root_child_id] = root_id;
    new_root.depth = cells[root_id].depth - 1;
    cells[root_id].parent = new_root_id;
    cells[root_id].child_index = root_child_id;

    const gfxm::vec3 extents(cells[root_id].aabb.to.x - cells[root_id].aabb.from.x, cells[root_id].aabb.to.y - cells[root_id].aabb.from.y, cells[root_id].aabb.to.z - cells[root_id].aabb.from.z);
    const gfxm::vec3 id_mods(
      (root_child_id & 1) - .5f,
      ((root_child_id & 2) >> 1) - .5f,
      ((root_child_id & 4) >> 2) - .5f
    );

    new_root.aabb = gfxm::aabb(
        gfxm::vec3(
            cells[root_id].aabb.from.x - (id_mods.x + .5f) * extents.x,
            cells[root_id].aabb.from.y - (id_mods.y + .5f) * extents.y,
            cells[root_id].aabb.from.z - (id_mods.z + .5f) * extents.z
        ),
        gfxm::vec3(
            cells[root_id].aabb.to.x - (id_mods.x - .5f) * extents.x,
            cells[root_id].aabb.to.y - (id_mods.y - .5f) * extents.y,
            cells[root_id].aabb.to.z - (id_mods.z - .5f) * extents.z
        )  
    );
    root_id = new_root_id;    
}

int32_t Octree::selectChild(int32_t cell, const gfxm::aabb& aabb) {
    if(cells[cell].depth == max_depth) {
        return cell;
    }
    const gfxm::vec3 center = (cells[cell].aabb.from + cells[cell].aabb.to) * .5f;
    char child_min = 0;
    char child_max = 0;

    child_min |= aabb.from.x > center.x ? 1 : 0;
    child_min |= aabb.from.y > center.y ? 2 : 0;
    child_min |= aabb.from.z > center.z ? 4 : 0;

    child_max |= aabb.to.x > center.x ? 1 : 0;
    child_max |= aabb.to.y > center.y ? 2 : 0;
    child_max |= aabb.to.z > center.z ? 4 : 0;

    if(child_min == child_max) {
        int32_t c_id = cells[cell].children[child_min];
        if(c_id == -1) {
            c_id = acquireAsChildCell(child_min, cell);
            cells[cell].children[child_min] = c_id;
        }
        return cells[cell].children[child_min];
    } else {
        return cell;
    }
}

void Octree::tryRemoveCell(int32_t cell) {
    while(cells[cell].objects.empty()) {
        if (cell == root_id) {
            return;
        }
        int32_t c = cell;
        for(size_t i = 0; i < 8; ++i) {
            if(cells[c].children[i] >= 0) {
                return;
            } 
        }
        cell = cells[c].parent;
        freeCell(c);
    }
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

    int32_t selected_cell = root_id;
    while(true) {
        int32_t prev_cell = selected_cell;
        selected_cell = selectChild(selected_cell, aabb);
        if(selected_cell == prev_cell) {
            break;
        }
    }

    return selected_cell;
}

int32_t Octree::createObject(const gfxm::aabb& box, void* user_ptr) {
    int32_t obj_id = acquireObject();
    objects[obj_id].aabb = box;

    auto cell_id = fit(box);
    objects[obj_id].cell = cell_id;
    objects[obj_id].user_ptr = user_ptr;
    cells[cell_id].objects.insert(obj_id);
    return obj_id;
}
void    Octree::deleteObject(int32_t uid) {
    int32_t cell = objects[uid].cell;
    cells[cell].objects.erase(uid);
    tryRemoveCell(cell);

    free_objects.insert(uid);
    active_objects.erase(uid);
}
void    Octree::updateObject(int32_t uid, const gfxm::aabb& box) {
    int32_t new_cell_id = fit(box);
    objects[uid].aabb = box;
    if(new_cell_id == objects[uid].cell) {
        return;
    }

    int32_t old_cell_id = objects[uid].cell;

    cells[old_cell_id].objects.erase(uid);
    cells[new_cell_id].objects.insert(uid);
    objects[uid].cell = new_cell_id;

    tryRemoveCell(old_cell_id);
}
OctreeObject* Octree::getObject(int32_t uid) {
    return &objects[uid];
}

#include <stack>

std::list<int32_t> Octree::listVisibleCells(const gfxm::frustum& fru) {
    std::list<int32_t> ret;

    std::stack<int32_t> stack;
    stack.push(root_id);
    while(!stack.empty()) {
        int32_t c = stack.top();
        stack.pop();
        if(gfxm::frustum_vs_aabb(fru, cells[c].aabb)) {
            if(!cells[c].objects.empty()) ret.insert(ret.end(), c);
            
            for(int i = 0; i < 8; ++i) {
                int32_t cc = cells[c].children[i];
                if(cc >= 0) {
                    stack.push(cc);
                }
            }
        }
    }

    return ret;
}

Octree::Cell*   Octree::getCell(int32_t id) {
    return &cells[id];
}

void Octree::debugDraw(DebugDraw& dd) {
    std::stack<int32_t> stack;
    stack.push(root_id);
    while(!stack.empty()) {
        int32_t c = stack.top();
        stack.pop();
        dd.aabb(cells[c].aabb, gfxm::vec3(1.0f, 0.7f, 0.0f));
        for(size_t i = 0; i < 8; ++i) {
            int32_t cc = cells[c].children[i];
            if(cc >= 0) {
                stack.push(cc);
            }
        }
    }

    for(auto id : active_objects) {
        dd.aabb(objects[id].aabb, gfxm::vec3(1,0,0));
    }
}