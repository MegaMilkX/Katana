#ifndef RESOURCE_MESH_HPP
#define RESOURCE_MESH_HPP

#include "resource.h"
#include "../gl/indexed_mesh.hpp"

class Mesh : public Resource {
    RTTR_ENABLE(Resource)
public:
    struct SubMesh {
        uint32_t indexOffset = 0; // BYTE OFFSET
        uint32_t indexCount = 0;
    };

    gl::IndexedMesh mesh;
    gfxm::aabb aabb;
    std::vector<SubMesh> submeshes;

    const float* getPermanentVertexData();
    const uint32_t* getPermanentIndexData();
    size_t vertexCount();
    size_t indexCount();

    void makeAabb() {
       std::vector<gfxm::vec3> vertices;
       vertices.resize(mesh.getAttribDataSize(gl::POSITION) / sizeof(gfxm::vec3));
       mesh.copyAttribData(gl::POSITION, vertices.data());

       if (!vertices.empty()) {
          aabb.from = vertices[0];
          aabb.to = vertices[0];
          for (size_t i = 0; i < vertices.size(); ++i) {
             auto& v = vertices[i];
             if (v.x < aabb.from.x) aabb.from.x = v.x;
             if (v.y < aabb.from.y) aabb.from.y = v.y;
             if (v.z < aabb.from.z) aabb.from.z = v.z;
             if (v.x > aabb.to.x) aabb.to.x = v.x;
             if (v.y > aabb.to.y) aabb.to.y = v.y;
             if (v.z > aabb.to.z) aabb.to.z = v.z;
          }
       }
    }

    virtual void serialize(out_stream& out) {
        assert(submeshes.size() < 255);
        out.write<uint8_t>((uint8_t)submeshes.size());
        for(unsigned i = 0; i < submeshes.size(); ++i) {
            out.write<uint32_t>(submeshes[i].indexOffset);
            out.write<uint32_t>(submeshes[i].indexCount);
        }
        mesh.serialize(out);
    }
    virtual bool deserialize(in_stream& in, size_t sz) { 
        submeshes.resize(in.read<uint8_t>());
        for(unsigned i = 0; i < submeshes.size(); ++i) {
            submeshes[i].indexOffset = in.read<uint32_t>();
            submeshes[i].indexCount = in.read<uint32_t>();
        }
        mesh.deserialize(in);

        makeAabb();

        return true; 
    }

    virtual const char* getWriteExtension() const { return "msh"; }
private:
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
};
STATIC_RUN(Mesh) {
    rttr::registration::class_<Mesh>("Mesh")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
