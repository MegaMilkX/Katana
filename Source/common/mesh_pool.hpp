#ifndef MESH_POOL_HPP
#define MESH_POOL_HPP

#include <vector>
#include <memory>
#include "gl/indexed_mesh.hpp"

enum PRIMITIVE_MESH {
    PRIM_CUBE   = 0,
    PRIM_PLANE  = 1,
    PRIM_MESH_COUNT
};

class MeshPool {
    static std::vector<std::unique_ptr<gl::IndexedMesh>> meshes;
public:
    static void init();
    static void cleanup();

    static gl::IndexedMesh* get(PRIMITIVE_MESH prim);
};


#endif
