#include "gen_lightmap_uv.hpp"

#include "../../lib/xatlas/xatlas.h"


void GenLightmapUV(Mesh* mesh) {
    auto& pos_desc = VFMT::GENERIC::getAttribDesc(VFMT::ENUM_GENERIC::Position);
    int vertexCount = mesh->mesh.getAttribDataSize(VFMT::ENUM_GENERIC::Position) / pos_desc.elem_size / pos_desc.count;
    int indexCount = mesh->mesh.getIndexCount();
    assert(vertexCount);
    assert(indexCount);

    std::vector<float> vertices(vertexCount * 3);
    std::vector<uint32_t> indices(indexCount);
    mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::Position)->extractData(vertices.data(), 0, vertexCount * pos_desc.elem_size * pos_desc.count);
    mesh->mesh.copyIndexData(indices.data());

    std::map<int, std::vector<char>> attribs_orig;
    for(int i = 0; i < VFMT::GENERIC::attribCount(); ++i) {
        if(i == VFMT::ENUM_GENERIC::UVLightmap) {
            continue;
        }
        auto attrib_buf = mesh->mesh.getAttribBuffer(i);
        if(!attrib_buf) {
            continue;
        }

        auto& attrib_bytes = attribs_orig[i];
        attrib_bytes.resize(attrib_buf->getDataSize());
        attrib_buf->extractData(attrib_bytes.data(), 0, attrib_buf->getDataSize());
    }

    // Make uvs
    xatlas::Atlas* atlas = xatlas::Create();
    assert(atlas);

    xatlas::MeshDecl meshDecl = { 0 };
    meshDecl.vertexCount = vertices.size() / 3;
    meshDecl.indexCount = indices.size();
    meshDecl.vertexPositionData = vertices.data();
    meshDecl.vertexPositionStride = sizeof(float) * 3;
    meshDecl.indexData = indices.data();
    meshDecl.indexFormat = xatlas::IndexFormat::UInt32;

    auto ret = xatlas::AddMesh(atlas, meshDecl);
    assert(ret == xatlas::AddMeshError::Success);

    xatlas::ComputeCharts(atlas);
    xatlas::ParameterizeCharts(atlas);
    xatlas::PackCharts(atlas);

    // Reupload new data
    std::map<int, std::vector<char>> attribs_new;
    std::vector<uint32_t> indices_new;
    std::vector<gfxm::vec2> xuv_layer;
    for(int i = 0; i < atlas->meshCount; ++i) {
        int xvertexCount = atlas->meshes[i].vertexCount;
        int xindexCount = atlas->meshes[i].indexCount;

        int vertex_offset = 0;
        for(int j = 0; j < VFMT::GENERIC::attribCount(); ++j) {
            if(j == VFMT::ENUM_GENERIC::UVLightmap) {
                continue;
            }
            const auto& attrib_desc = VFMT::GENERIC::getAttribDesc(j);
            int attrib_elem_size = attrib_desc.elem_size;

            auto attrib_buf = mesh->mesh.getAttribBuffer(j);
            if(!attrib_buf) {
                continue;
            }
            auto& old_buf = attribs_orig[j];
            auto& new_buf = attribs_new[j];

            int attribPerVertexSz = attrib_elem_size * attrib_desc.count;
            new_buf.resize(new_buf.size() + attribPerVertexSz * xvertexCount);

            xatlas::Vertex* xvertices = atlas->meshes[i].vertexArray;
            for(int k = 0; k < xvertexCount; ++k) {
                memcpy(
                    &new_buf[vertex_offset * attribPerVertexSz + k * attribPerVertexSz],
                    &old_buf[xvertices[k].xref * attribPerVertexSz],
                    attribPerVertexSz
                );
            }
        }

        // lightmap uv and position data
        xuv_layer.resize(xuv_layer.size() + xvertexCount);
        xatlas::Vertex* xvertices = atlas->meshes[i].vertexArray;
        for(int j = 0; j < xvertexCount; ++j) {
            xuv_layer[vertex_offset + j].x = xvertices[j].uv[0] / atlas->width;
            xuv_layer[vertex_offset + j].y = xvertices[j].uv[1] / atlas->height;
        }

        for (int j = 0; j < xindexCount; ++j) {
            indices_new.push_back(vertex_offset + atlas->meshes[i].indexArray[j]);
        }

        vertex_offset += xvertexCount;
    }

    // Upload new data
    for(int i = 0; i < VFMT::GENERIC::attribCount(); ++i) {
        if(i == VFMT::ENUM_GENERIC::UVLightmap) {
            continue;
        }
        auto it = attribs_new.find(i);
        if (it == attribs_new.end()) {
            continue;
        }
        auto& attrib_bytes = it->second;
        
        mesh->mesh.setAttribData(i, attrib_bytes.data(), attrib_bytes.size());
    }
    mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::UVLightmap, xuv_layer.data(), xuv_layer.size() * sizeof(xuv_layer[0]));
    mesh->mesh.setIndices(indices_new.data(), indices_new.size());
}