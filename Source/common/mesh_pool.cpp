#include "mesh_pool.hpp"


std::vector<std::unique_ptr<gl::IndexedMesh>> MeshPool::meshes;

void MeshPool::init() {
    meshes.resize(PRIM_MESH_COUNT);

    gl::IndexedMesh* m = new gl::IndexedMesh();
    std::vector<float> vertices = {
        -0.5f, -0.5f, 0.5f,
         0.5f, -0.5f, 0.5f,
         0.5f,  0.5f, 0.5f,
        -0.5f,  0.5f, 0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    std::vector<uint32_t> indices = {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        0, 4, 7, 0, 7, 3,
        1, 6, 5, 1, 2, 6,
        0, 5, 4, 0, 1, 5,
        3, 6, 2, 3, 7, 6
    };
    m->setAttribData(VERTEX_FMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(vertices[0]));
    m->setIndices(indices.data(), indices.size());
    meshes[PRIM_CUBE].reset(m);
}
void MeshPool::cleanup() {
    meshes.clear();
}

gl::IndexedMesh* MeshPool::get(PRIMITIVE_MESH prim) {
    return meshes[prim].get();
}