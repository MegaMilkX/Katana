#ifndef MESH_DEFORMER_HPP
#define MESH_DEFORMER_HPP

#include "../../gl/vertex_array_object.hpp"
#include "../../gl/vertex_buffer.hpp"
#include "../../gl/buffer.hpp"

#include "../../gl/indexed_mesh.hpp"


enum MESH_DEFORMER {
    MESH_DEFORMER_NONE = -1,

    MESH_DEFORMER_SKIN,
    MESH_DEFORMER_WAVE,

    MESH_DEFORMER_COUNT
};


class MeshDeformerBase {
    MESH_DEFORMER       type;
    gl::IndexedMesh*    source_mesh;
public:
    MeshDeformerBase(MESH_DEFORMER type, gl::IndexedMesh* mesh)
    : type(type), source_mesh(mesh) {}

    MESH_DEFORMER getType() const { return type; }
    gl::IndexedMesh* getSourceMesh() { return source_mesh; }
};


class Model_;
class MeshDeformerSkin : public MeshDeformerBase {
public:
    std::unique_ptr<gl::VertexArrayObject>  vao;
    std::unique_ptr<gl::Buffer>             position;
    std::unique_ptr<gl::Buffer>             normal;
    std::unique_ptr<gl::Buffer>             tangent;
    std::unique_ptr<gl::Buffer>             bitangent;
    
    std::unique_ptr<gl::Buffer>             pose;

    std::vector<int>            bone_nodes;
    std::vector<gfxm::mat4>     bind_transforms;

    Model_* model;

    MeshDeformerSkin(Model_* model, gl::IndexedMesh* mesh, const std::vector<int>& bone_nodes, const std::vector<gfxm::mat4>& bind_transforms) 
    : MeshDeformerBase(MESH_DEFORMER_SKIN, mesh), model(model), bone_nodes(bone_nodes), bind_transforms(bind_transforms) {
        vao.reset(new gl::VertexArrayObject);

        position.reset(new gl::Buffer(
            GL_STREAM_DRAW, mesh->getAttribDataSize(VFMT::ENUM_GENERIC::Position)
        ));
        normal.reset(new gl::Buffer(
            GL_STREAM_DRAW, mesh->getAttribDataSize(VFMT::ENUM_GENERIC::Normal)
        ));
        tangent.reset(new gl::Buffer(
            GL_STREAM_DRAW, mesh->getAttribDataSize(VFMT::ENUM_GENERIC::Tangent)
        ));
        bitangent.reset(new gl::Buffer(
            GL_STREAM_DRAW, mesh->getAttribDataSize(VFMT::ENUM_GENERIC::Bitangent)
        ));
        vao->attach(position->getId(), VFMT::ENUM_GENERIC::Position, VFMT::PositionDesc);
        vao->attach(normal->getId(), VFMT::ENUM_GENERIC::Normal, VFMT::NormalDesc);
        vao->attach(tangent->getId(), VFMT::ENUM_GENERIC::Tangent, VFMT::TangentDesc);
        vao->attach(bitangent->getId(), VFMT::ENUM_GENERIC::Bitangent, VFMT::BitangentDesc);

        auto buf = mesh->getAttribBuffer(VFMT::ENUM_GENERIC::UV);
        if(buf) vao->attach(buf->getId(), VFMT::ENUM_GENERIC::UV, VFMT::UVDesc);
        buf = mesh->getAttribBuffer(VFMT::ENUM_GENERIC::UVLightmap);
        if(buf) vao->attach(buf->getId(), VFMT::ENUM_GENERIC::UVLightmap, VFMT::UVLightmapDesc);
        buf = mesh->getAttribBuffer(VFMT::ENUM_GENERIC::ColorRGBA);
        if(buf) vao->attach(buf->getId(), VFMT::ENUM_GENERIC::ColorRGBA, VFMT::ColorRGBADesc);

        vao->attachIndexBuffer(mesh->getIndexBuffer()->getId());

        pose.reset(new gl::Buffer(
            GL_STATIC_DRAW, sizeof(float) * 16 * bone_nodes.size()
        ));
    }
    

};


#endif
