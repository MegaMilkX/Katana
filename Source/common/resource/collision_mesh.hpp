#ifndef RESOURCE_COLLISION_MESH_HPP
#define RESOURCE_COLLISION_MESH_HPP

#include "resource.h"
#include "../gfxm.hpp"


class CollisionMesh : public Resource {
    RTTR_ENABLE(Resource)

    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t>   indices;

public:
    void setVertices(const gfxm::vec3* ptr, uint64_t count) {
        vertices = std::vector<gfxm::vec3>(ptr, ptr + count);
    }
    void setIndices(const uint32_t* ptr, uint64_t count) {
        indices = std::vector<uint32_t>(ptr, ptr + count);
    }

    int vertexCount() const { return vertices.size(); }
    int indexCount()  const { return indices.size();  }
    gfxm::vec3* getVertexDataPtr() { return vertices.data(); }
    uint32_t*   getIndexDataPtr()  { return indices.data();  }

    void serialize(out_stream& out) override {
        out.write<uint64_t>(vertices.size());
        out.write<uint64_t>(indices.size());
        out.write(vertices.data(), vertices.size() * sizeof(vertices[0]));
        out.write(indices.data(), indices.size() * sizeof(indices[0]));
    }
    bool deserialize(in_stream& in, size_t sz) override {
        uint64_t vertex_count = 0;
        uint64_t index_count = 0;
        in.read(vertex_count);
        in.read(index_count);
        in.read(vertices, vertex_count);
        in.read(indices, index_count);
        return true;
    }
};
STATIC_RUN(CollisionMesh) {
    rttr::registration::class_<CollisionMesh>("CollisionMesh")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}


#endif
