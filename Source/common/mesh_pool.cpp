#include "mesh_pool.hpp"


std::vector<std::unique_ptr<gl::IndexedMesh>> MeshPool::meshes;

void MeshPool::init() {
    meshes.resize(PRIM_MESH_COUNT);

    gl::IndexedMesh* m = new gl::IndexedMesh();
    std::vector<float> vertices = {
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f
    };
    std::vector<gfxm::vec3> normals = {
        gfxm::normalize(gfxm::vec3(-0.5f, -0.5f,  0.5f)),
        gfxm::normalize(gfxm::vec3( 0.5f, -0.5f,  0.5f)),
        gfxm::normalize(gfxm::vec3( 0.5f,  0.5f,  0.5f)),
        gfxm::normalize(gfxm::vec3(-0.5f,  0.5f,  0.5f)),
        gfxm::normalize(gfxm::vec3(-0.5f, -0.5f, -0.5f)),
        gfxm::normalize(gfxm::vec3( 0.5f, -0.5f, -0.5f)),
        gfxm::normalize(gfxm::vec3( 0.5f,  0.5f, -0.5f)),
        gfxm::normalize(gfxm::vec3(-0.5f,  0.5f, -0.5f))
    };
    std::vector<gfxm::vec3> bitangents = {
        gfxm::cross(normals[0], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[1], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[2], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[3], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[4], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[5], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[6], gfxm::vec3(0, 1, 0)),
        gfxm::cross(normals[7], gfxm::vec3(0, 1, 0))
    };
    std::vector<gfxm::vec3> tangents = {
        gfxm::cross(normals[0], bitangents[0]),
        gfxm::cross(normals[1], bitangents[1]),
        gfxm::cross(normals[2], bitangents[2]),
        gfxm::cross(normals[3], bitangents[3]),
        gfxm::cross(normals[4], bitangents[4]),
        gfxm::cross(normals[5], bitangents[5]),
        gfxm::cross(normals[6], bitangents[6]),
        gfxm::cross(normals[7], bitangents[7])
    };/*
    std::vector<uint8_t> color_rgba = {
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 0, 255, 255,
        255, 0, 0, 255,
        0, 255, 0, 255,
        0, 0, 255, 255,
        255, 0, 255, 255,
    };*/
    std::vector<uint8_t> color_rgba = {
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255,
        255, 255, 255, 255
    };
    std::vector<uint32_t> indices = {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        0, 4, 7, 0, 7, 3,
        1, 6, 5, 1, 2, 6,
        0, 5, 4, 0, 1, 5,
        3, 6, 2, 3, 7, 6
    };
    m->setAttribData(VFMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(vertices[0]));
    m->setAttribData(VFMT::ENUM_GENERIC::Normal, normals.data(), normals.size() * sizeof(normals[0]));
    m->setAttribData(VFMT::ENUM_GENERIC::Tangent, tangents.data(), tangents.size() * sizeof(tangents[0]));
    m->setAttribData(VFMT::ENUM_GENERIC::Bitangent, bitangents.data(), bitangents.size() * sizeof(bitangents[0]));
    m->setAttribData(VFMT::ENUM_GENERIC::ColorRGBA, color_rgba.data(), color_rgba.size() * sizeof(vertices[0]));
    m->setIndices(indices.data(), indices.size());
    meshes[PRIM_CUBE].reset(m);
}
void MeshPool::cleanup() {
    meshes.clear();
}

gl::IndexedMesh* MeshPool::get(PRIMITIVE_MESH prim) {
    return meshes[prim].get();
}