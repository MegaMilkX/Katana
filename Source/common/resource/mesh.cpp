#include "mesh.hpp"

const float* Mesh::getPermanentVertexData() {
    if(vertexCount() == 0) return 0;
    if(vertices.size() == 0) {
        vertices.resize(vertexCount() * 3);
        mesh.copyAttribData(VFMT::ENUM_GENERIC::Position, vertices.data());
    }
    return vertices.data();
}
const uint32_t* Mesh::getPermanentIndexData() {
    if(indexCount() == 0) return 0;
    if(indices.size() == 0) {
        indices.resize(indexCount());
        mesh.copyIndexData(indices.data());
    }
    return indices.data();
}
size_t Mesh::vertexCount() {
    auto& desc = VFMT::GENERIC::getAttribDesc(VFMT::ENUM_GENERIC::Position);
    return mesh.getAttribDataSize(VFMT::ENUM_GENERIC::Position) / (desc.elem_size * desc.count);
}
size_t Mesh::indexCount() {
    return mesh.getIndexCount();
}