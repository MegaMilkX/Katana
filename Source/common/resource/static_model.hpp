#ifndef STATIC_MESH_HPP
#define STATIC_MESH_HPP

#include "resource.h"
#include "../gl/indexed_mesh.hpp"


class StaticModel : public Resource {
    RTTR_ENABLE(Resource)
public:
    std::vector<gfxm::mat4> transforms;
    std::vector<std::unique_ptr<gl::IndexedMesh>> meshes;

    void serialize(out_stream& out) override {
        out.write<uint32_t>(meshes.size());
        for(int i = 0; i < meshes.size(); ++i) {
            meshes[i]->serialize(out);
        }
    }
    bool deserialize(in_stream& in, size_t sz) override {
        meshes.resize(in.read<uint32_t>());
        for(int i = 0; i < meshes.size(); ++i) {
            meshes[i].reset(new gl::IndexedMesh());
            meshes[i]->deserialize(in);
        }
        return true;
    }
    const char* getWriteExtension() const override { return "static_model"; }
};
STATIC_RUN(StaticModel) {
    rttr::registration::class_<StaticModel>("StaticModel")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
