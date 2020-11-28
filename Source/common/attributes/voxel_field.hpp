#ifndef VOXEL_FIELD_HPP
#define VOXEL_FIELD_HPP

#include "renderable_base.hpp"
#include "../scene/node.hpp"

#include "../gl/indexed_mesh.hpp"

#include "../util/marching_cubes/tables.hpp"

#include "../resource/material.hpp"

#include "../util/imgui_helpers.hpp"

#include "../lib/FastNoiseSIMD/FastNoiseSIMD.h"

const int fieldSize = 16;
const int voxelArraySize = fieldSize * fieldSize * fieldSize;

inline gfxm::vec2 sampleSphericalMap(gfxm::vec3 v) {
    const gfxm::vec2 invAtan = gfxm::vec2(0.1591f, 0.3183f);
    gfxm::vec2 uv = gfxm::vec2(atan2f(v.z, v.x), asinf(v.y));
    uv *= invAtan;
    uv = gfxm::vec2(uv.x + 0.5, uv.y + 0.5);
    return uv;
}

inline size_t indexFromCoord(int x, int y, int z, int sideSize) {
    return x * sideSize * sideSize + y * sideSize + z;
}

class VoxelField : public RenderableBase {
    RTTR_ENABLE(RenderableBase)

    float voxelField[voxelArraySize];
    gl::IndexedMesh mesh;
    float threshold = 0.1f;

    std::shared_ptr<Material> material;

    gfxm::vec3 interpolateEdgeVertex(const gfxm::vec3& v1, const gfxm::vec3& v2, float a, float b) {
        float mu = (threshold - a) / (b - a);
        gfxm::vec3 p;
        p.x = v1.x + mu * (float) (v2.x - v1.x);
        p.y = v1.y + mu * (float) (v2.y - v1.y);
        p.z = v1.z + mu * (float) (v2.z - v1.z);
        return p;
    }

    void rebuildMesh() {
        std::vector<float> vertices;
        std::vector<float> normals;
        std::vector<gfxm::vec3> tangents;
        std::vector<gfxm::vec3> bitangents;
        std::vector<float> uv;
        std::vector<uint32_t> indices;

        std::vector<gfxm::vec3> edge_vertex_grid;
        edge_vertex_grid.resize(fieldSize*fieldSize*fieldSize*3);
        std::vector<gfxm::vec3> edge_vertex_normal_grid;
        edge_vertex_normal_grid.resize(fieldSize*fieldSize*fieldSize*3);
        std::vector<gfxm::vec2> edge_vertex_uv_grid;
        edge_vertex_uv_grid.resize(fieldSize*fieldSize*fieldSize*3);

        for(int z = 0; z < fieldSize - 1; ++z) {
            for(int y = 0; y < fieldSize - 1; ++y) {
                for(int x = 0; x < fieldSize - 1; ++x) {
                    size_t cell_idx = indexFromCoord(x, y, z, fieldSize);
                    // ngb - short for neighbor
                    size_t cell_idx_x_ngb = indexFromCoord(x + 1, y,     z, fieldSize);
                    size_t cell_idx_z_ngb = indexFromCoord(x,     y,     z + 1, fieldSize);
                    size_t cell_idx_y_ngb = indexFromCoord(x,     y + 1, z, fieldSize);
                    size_t cell_idx_xy_ngb = indexFromCoord(x + 1,     y + 1, z, fieldSize);
                    size_t cell_idx_yz_ngb = indexFromCoord(x,     y + 1, z + 1, fieldSize);
                    size_t cell_idx_xz_ngb = indexFromCoord(x + 1,     y, z + 1, fieldSize);
                    size_t arrayIndices[] = {
                        indexFromCoord(x, y, z, fieldSize),
                        indexFromCoord(x + 1, y, z, fieldSize),
                        indexFromCoord(x + 1, y, z + 1, fieldSize),
                        indexFromCoord(x, y, z + 1, fieldSize),
                        indexFromCoord(x, y + 1, z, fieldSize),
                        indexFromCoord(x + 1, y + 1, z, fieldSize),
                        indexFromCoord(x + 1, y + 1, z + 1, fieldSize),
                        indexFromCoord(x, y + 1, z + 1, fieldSize)
                    };
                    float densities[8];
                    const gfxm::vec3 corners[] = {
                        {  .0f + x,  .0f + y,  .0f + z },
                        { 1.0f + x,  .0f + y,  .0f + z },
                        { 1.0f + x,  .0f + y, 1.0f + z },
                        {  .0f + x,  .0f + y, 1.0f + z },
                        {  .0f + x, 1.0f + y,  .0f + z },
                        { 1.0f + x, 1.0f + y,  .0f + z },
                        { 1.0f + x, 1.0f + y, 1.0f + z },
                        {  .0f + x, 1.0f + y, 1.0f + z }
                    };
                    size_t cubeIndex = 0;
                    for(int i = 0; i < 8; ++i) {
                        densities[i] = voxelField[arrayIndices[i]];
                        if(voxelField[arrayIndices[i]] > threshold) {
                            cubeIndex |= 1 << i;
                        }
                    }

                    if(edgeTable[cubeIndex] == 0 || edgeTable[cubeIndex] == 255) continue;

                    gfxm::vec3 edge_vertices[12];

                    const int local_vertex_to_edge_grid[] = {
                        cell_idx * 3 + 0,
                        cell_idx_x_ngb * 3 + 1,
                        cell_idx_z_ngb * 3 + 0,
                        cell_idx * 3 + 1,
                        cell_idx_y_ngb * 3 + 0,
                        cell_idx_xy_ngb * 3 + 1,
                        cell_idx_yz_ngb * 3 + 0,
                        cell_idx_y_ngb * 3 + 1,
                        cell_idx * 3 + 2,
                        cell_idx_x_ngb * 3 + 2,
                        cell_idx_xz_ngb * 3 + 2,
                        cell_idx_z_ngb * 3 + 2
                    };

                    if ((edgeTable[cubeIndex] & 1) == 1)
                        edge_vertex_grid[local_vertex_to_edge_grid[0]] = interpolateEdgeVertex(corners[0], corners[1], densities[0], densities[1]);
                    if ((edgeTable[cubeIndex] & 2) == 2)
                        edge_vertex_grid[local_vertex_to_edge_grid[1]] = interpolateEdgeVertex(corners[1], corners[2], densities[1], densities[2]);
                    if ((edgeTable[cubeIndex] & 4) == 4)
                        edge_vertex_grid[local_vertex_to_edge_grid[2]] = interpolateEdgeVertex(corners[2], corners[3], densities[2], densities[3]);
                    if ((edgeTable[cubeIndex] & 8) == 8)
                        edge_vertex_grid[local_vertex_to_edge_grid[3]] = interpolateEdgeVertex(corners[3], corners[0], densities[3], densities[0]);
                    if ((edgeTable[cubeIndex] & 16) == 16)
                        edge_vertex_grid[local_vertex_to_edge_grid[4]] = interpolateEdgeVertex(corners[4], corners[5], densities[4], densities[5]);
                    if ((edgeTable[cubeIndex] & 32) == 32)
                        edge_vertex_grid[local_vertex_to_edge_grid[5]] = interpolateEdgeVertex(corners[5], corners[6], densities[5], densities[6]);
                    if ((edgeTable[cubeIndex] & 64) == 64)
                        edge_vertex_grid[local_vertex_to_edge_grid[6]] = interpolateEdgeVertex(corners[6], corners[7], densities[6], densities[7]);
                    if ((edgeTable[cubeIndex] & 128) == 128)
                        edge_vertex_grid[local_vertex_to_edge_grid[7]] = interpolateEdgeVertex(corners[7], corners[4], densities[7], densities[4]);
                    if ((edgeTable[cubeIndex] & 256) == 256)
                        edge_vertex_grid[local_vertex_to_edge_grid[8]] = interpolateEdgeVertex(corners[0], corners[4], densities[0], densities[4]);
                    if ((edgeTable[cubeIndex] & 512) == 512)
                        edge_vertex_grid[local_vertex_to_edge_grid[9]] = interpolateEdgeVertex(corners[1], corners[5], densities[1], densities[5]);
                    if ((edgeTable[cubeIndex] & 1024) == 1024)
                        edge_vertex_grid[local_vertex_to_edge_grid[10]] = interpolateEdgeVertex(corners[2], corners[6], densities[2], densities[6]);
                    if ((edgeTable[cubeIndex] & 2048) == 2048)
                        edge_vertex_grid[local_vertex_to_edge_grid[11]] = interpolateEdgeVertex(corners[3], corners[7], densities[3], densities[7]);

                    for(int i = 0; i < 16; ++i) {
                        int vertex_idx = triTable[cubeIndex][i];
                        if(vertex_idx < 0) break;
                        
                        indices.emplace_back(local_vertex_to_edge_grid[vertex_idx]);
                    }

                    /*
                    if ((edgeTable[cubeIndex] & 1) == 1)
                        edge_vertices[0] = interpolateEdgeVertex(corners[0], corners[1], densities[0], densities[1]);
                    if ((edgeTable[cubeIndex] & 2) == 2)
                        edge_vertices[1] = interpolateEdgeVertex(corners[1], corners[2], densities[1], densities[2]);
                    if ((edgeTable[cubeIndex] & 4) == 4)
                        edge_vertices[2] = interpolateEdgeVertex(corners[2], corners[3], densities[2], densities[3]);
                    if ((edgeTable[cubeIndex] & 8) == 8)
                        edge_vertices[3] = interpolateEdgeVertex(corners[3], corners[0], densities[3], densities[0]);
                    if ((edgeTable[cubeIndex] & 16) == 16)
                        edge_vertices[4] = interpolateEdgeVertex(corners[4], corners[5], densities[4], densities[5]);
                    if ((edgeTable[cubeIndex] & 32) == 32)
                        edge_vertices[5] = interpolateEdgeVertex(corners[5], corners[6], densities[5], densities[6]);
                    if ((edgeTable[cubeIndex] & 64) == 64)
                        edge_vertices[6] = interpolateEdgeVertex(corners[6], corners[7], densities[6], densities[7]);
                    if ((edgeTable[cubeIndex] & 128) == 128)
                        edge_vertices[7] = interpolateEdgeVertex(corners[7], corners[4], densities[7], densities[4]);
                    if ((edgeTable[cubeIndex] & 256) == 256)
                        edge_vertices[8] = interpolateEdgeVertex(corners[0], corners[4], densities[0], densities[4]);
                    if ((edgeTable[cubeIndex] & 512) == 512)
                        edge_vertices[9] = interpolateEdgeVertex(corners[1], corners[5], densities[1], densities[5]);
                    if ((edgeTable[cubeIndex] & 1024) == 1024)
                        edge_vertices[10] = interpolateEdgeVertex(corners[2], corners[6], densities[2], densities[6]);
                    if ((edgeTable[cubeIndex] & 2048) == 2048)
                        edge_vertices[11] = interpolateEdgeVertex(corners[3], corners[7], densities[3], densities[7]);

                    static const gfxm::vec2 edge_uv[] = {
                        { 0.5f, .0f },
                        { 1.0f, .0f },
                        { 0.5f, .0f },
                        {  .0f, .0f },
                        { 0.5f, 1.0f },
                        { 1.0f, 1.0f },
                        { 0.5f, 1.0f },
                        {  .0f, 1.0f },
                        {  .0f, 0.5f },
                        { 1.0f, 0.5f },
                        { 1.0f, 0.5f },
                        {  .0f, 0.5f }
                    };

                    for(int i = 0; i < 16; ++i) {
                        int vertex_idx = triTable[cubeIndex][i];
                        if(vertex_idx < 0) break;
                        gfxm::vec3 vertex = edge_vertices[vertex_idx];
                        gfxm::vec2 u = edge_uv[vertex_idx];
                        vertex = vertex + gfxm::vec3(x, y, z);
                        vertices.emplace_back(vertex.x);
                        vertices.emplace_back(vertex.y);
                        vertices.emplace_back(vertex.z);
                        uv.emplace_back(u.x);
                        uv.emplace_back(u.y);
                        indices.emplace_back(indices.size());
                    }*/
                }
            }
        }

        // for each triangle
        for(size_t i = 0; i < indices.size(); i+=3) {
            size_t idx_a = indices[i];
            size_t idx_b = indices[i + 1];
            size_t idx_c = indices[i + 2];
            const gfxm::vec3& a = edge_vertex_grid[idx_a];
            const gfxm::vec3& b = edge_vertex_grid[idx_b];
            const gfxm::vec3& c = edge_vertex_grid[idx_c];
            gfxm::vec3 edge_a = (b - a);
            gfxm::vec3 edge_b = (c - a);
            gfxm::vec3 normal = gfxm::normalize(gfxm::cross(edge_a, edge_b));
            
            gfxm::vec3& na = edge_vertex_normal_grid[idx_a];
            gfxm::vec3& nb = edge_vertex_normal_grid[idx_b];
            gfxm::vec3& nc = edge_vertex_normal_grid[idx_c];


            na += normal;
            na = gfxm::normalize(na);
            nb += normal;
            nb = gfxm::normalize(nb);
            nc += normal;
            nc = gfxm::normalize(nc);
        }

        for(size_t i = 0; i < edge_vertex_normal_grid.size(); ++i) {
            gfxm::vec3 normal = edge_vertex_normal_grid[i];

            gfxm::vec2 u = sampleSphericalMap(normal);

            edge_vertex_uv_grid[i] = u;
        }

        tangents.resize(edge_vertex_grid.size());
        bitangents.resize(edge_vertex_grid.size());
        for(size_t i = 0; i < indices.size(); i+=3) {
            size_t idx_a = indices[i];
            size_t idx_b = indices[i + 1];
            size_t idx_c = indices[i + 2];

            gfxm::vec3 a = edge_vertex_grid[idx_a];
            gfxm::vec3 b = edge_vertex_grid[idx_b];
            gfxm::vec3 c = edge_vertex_grid[idx_c];
            gfxm::vec2 uva = edge_vertex_uv_grid[idx_a];
            gfxm::vec2 uvb = edge_vertex_uv_grid[idx_b];
            gfxm::vec2 uvc = edge_vertex_uv_grid[idx_c];

            gfxm::vec3 deltaPos1 = b - a;
            gfxm::vec3 deltaPos2 = c - a;

            gfxm::vec2 deltaUV1 = uvb - uva;
            gfxm::vec2 deltaUV2 = uvc - uva;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            gfxm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y) * r;
            gfxm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x) * r;
            tangent = gfxm::normalize(tangent);
            bitangent = gfxm::normalize(bitangent);

            tangents[idx_a] = tangent;
            tangents[idx_b] = tangent;
            tangents[idx_c] = tangent;

            bitangents[idx_a] = bitangent;
            bitangents[idx_b] = bitangent;
            bitangents[idx_c] = bitangent;
        }
    /*
        for(size_t i = 0; i < vertices.size() / 3 / 3; ++i) {
            size_t stride = 9;
            gfxm::vec3 a(vertices[i * stride], vertices[i * stride + 1], vertices[i * stride + 2]);
            gfxm::vec3 b(vertices[i * stride + 3], vertices[i * stride + 3 + 1], vertices[i * stride + 3 + 2]);
            gfxm::vec3 c(vertices[i * stride + 6], vertices[i * stride + 6 + 1], vertices[i * stride + 6 + 2]);
            gfxm::vec3 edge_a = (b - a);
            gfxm::vec3 edge_b = (c - a);
            gfxm::vec3 normal = gfxm::normalize(gfxm::cross(edge_a, edge_b));
            for(int j = 0; j < 3; ++j) {
                normals.emplace_back(normal.x); normals.emplace_back(normal.y); normals.emplace_back(normal.z);
            }
        }*/
/*
        for(size_t i = 0; i < vertices.size() / 3; ++i) {
            size_t stride = 3;
            gfxm::vec3 normal(normals[i * stride], normals[i * stride + 1], normals[i * stride + 2]);

            gfxm::vec2 u = sampleSphericalMap(normal);

            uv.emplace_back(u.x);
            uv.emplace_back(u.y);
        }
*//*
        for(size_t i = 0; i < vertices.size() / 3 / 3; ++i) {
            size_t stride = 9;
            size_t uv_stride = 6;
            gfxm::vec3 a(vertices[i * stride], vertices[i * stride + 1], vertices[i * stride + 2]);
            gfxm::vec3 b(vertices[i * stride + 3], vertices[i * stride + 3 + 1], vertices[i * stride + 3 + 2]);
            gfxm::vec3 c(vertices[i * stride + 6], vertices[i * stride + 6 + 1], vertices[i * stride + 6 + 2]);
            gfxm::vec2 uva(uv[i * uv_stride], uv[i * uv_stride + 1]);
            gfxm::vec2 uvb(uv[i * uv_stride + 2], uv[i * uv_stride + 2 + 1]);
            gfxm::vec2 uvc(uv[i * uv_stride + 4], uv[i * uv_stride + 4 + 1]);

            gfxm::vec3 deltaPos1 = b - a;
            gfxm::vec3 deltaPos2 = c - a;

            gfxm::vec2 deltaUV1 = uvb - uva;
            gfxm::vec2 deltaUV2 = uvc - uva;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            gfxm::vec3 tangent = (deltaPos1 * deltaUV2.y   - deltaPos2 * deltaUV1.y) * r;
            gfxm::vec3 bitangent = (deltaPos2 * deltaUV1.x   - deltaPos1 * deltaUV2.x) * r;
            tangent = gfxm::normalize(tangent);
            bitangent = gfxm::normalize(bitangent);

            tangents.emplace_back(tangent.x); tangents.emplace_back(tangent.y); tangents.emplace_back(tangent.z);
            tangents.emplace_back(tangent.x); tangents.emplace_back(tangent.y); tangents.emplace_back(tangent.z);
            tangents.emplace_back(tangent.x); tangents.emplace_back(tangent.y); tangents.emplace_back(tangent.z);

            bitangents.emplace_back(bitangent.x); bitangents.emplace_back(bitangent.y); bitangents.emplace_back(bitangent.z);
            bitangents.emplace_back(bitangent.x); bitangents.emplace_back(bitangent.y); bitangents.emplace_back(bitangent.z);
            bitangents.emplace_back(bitangent.x); bitangents.emplace_back(bitangent.y); bitangents.emplace_back(bitangent.z);
        }*/

        mesh.setAttribData(VFMT::ENUM_GENERIC::Position, edge_vertex_grid.data(), edge_vertex_grid.size() * sizeof(gfxm::vec3));
        mesh.setAttribData(VFMT::ENUM_GENERIC::Normal, edge_vertex_normal_grid.data(), edge_vertex_normal_grid.size() * sizeof(gfxm::vec3));
        mesh.setAttribData(VFMT::ENUM_GENERIC::Tangent, tangents.data(), tangents.size() * sizeof(gfxm::vec3));
        mesh.setAttribData(VFMT::ENUM_GENERIC::Bitangent, bitangents.data(), bitangents.size() * sizeof(gfxm::vec3));
        mesh.setAttribData(VFMT::ENUM_GENERIC::UV, edge_vertex_uv_grid.data(), edge_vertex_uv_grid.size() * sizeof(gfxm::vec2));
        mesh.setIndices(indices.data(), indices.size());
    }
public:
    VoxelField() {

        FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
        noise->FillSimplexFractalSet(voxelField, 0, 0, 0, fieldSize, fieldSize, fieldSize, 0.2f);
        delete noise;

        rebuildMesh();
    }

    void addToDrawList(DrawList& dl) override {
        DrawCmdSolid cmd;
        cmd.vao = mesh.getVao();
        cmd.transform = getOwner()->getTransform()->getWorldTransform();
        cmd.indexCount = mesh.getIndexCount();
        cmd.material = material.get();
        cmd.object_ptr = getOwner();
        dl.solids.emplace_back(cmd);
    }

    float noiseScale = 1.0f;
    gfxm::vec3 noiseOffset;
    void onGui() override {
        //noiseOffset = noiseOffset + gfxm::normalize(gfxm::vec3(1.0f, 1.0f, 1.0f)) * (1.0f/60.0f) * 100.0f;
        FastNoiseSIMD* noise = FastNoiseSIMD::NewFastNoiseSIMD();
        noise->SetNoiseType(FastNoiseSIMD::PerlinFractal);
        
        //noise->SetAxisScales(noiseScale, noiseScale, noiseScale);
        noise->SetFrequency(0.02f);
        
        
        float* ptr = noise->GetNoiseSet(noiseOffset.x,noiseOffset.y,noiseOffset.z, fieldSize, fieldSize, fieldSize, noiseScale);
        memcpy(voxelField, ptr, fieldSize*fieldSize*fieldSize*sizeof(float));
        noise->FreeNoiseSet(ptr);

        delete noise;
        rebuildMesh();

        if(ImGui::DragFloat("threshold", &threshold, 0.01f, 0.0f, 1.0f)) {
            rebuildMesh();
        }
        imguiResourceTreeCombo("material", material, "mat", [](){

        });

        if(ImGui::DragFloat("noise scale", &noiseScale, 0.01f, 0.0f)) {

        }
        if(ImGui::DragFloat3("offset", (float*)&noiseOffset)) {

        }
    }
};

#endif
