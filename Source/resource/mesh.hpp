#ifndef RESOURCE_MESH_HPP
#define RESOURCE_MESH_HPP

#include "resource.h"
#include "../gl/indexed_mesh.hpp"

class Mesh : public Resource {
public:
    gl::IndexedMesh mesh;

    virtual bool Build(DataSourceRef r) {
        return false;
    }
    virtual bool Serialize(std::vector<unsigned char>& data) {
        return false;
    }

    virtual void serialize(std::ostream& out) {
        mesh.serialize(out);
    }
    virtual bool deserialize(std::istream& in) { 
        mesh.deserialize(in);
        return true; 
    }
};
STATIC_RUN(Mesh) {
    rttr::registration::class_<Mesh>("Mesh")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif