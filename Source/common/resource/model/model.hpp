#ifndef RES_MODEL_HPP
#define RES_MODEL_HPP

#include "../resource.h"
#include "../../gl/vertex_array_object.hpp"
#include "../../gl/vertex_buffer.hpp"
#include "../../gl/buffer.hpp"
#include "../mesh.hpp"


class ModelNode {
public:
    std::string name;

    ModelNode* parent = 0;
    std::vector<std::unique_ptr<ModelNode>> children;

    gfxm::vec3 translation;
    gfxm::quat rotation;
    gfxm::vec3 scale = gfxm::vec3(1,1,1);
    gfxm::mat4 local;
    gfxm::mat4 world;

    const gfxm::mat4& getLocal() {
        return local = gfxm::translate(gfxm::mat4(1.0f), translation) *
            gfxm::to_mat4(rotation) *
            gfxm::scale(gfxm::mat4(1.0f), scale);
    }
    const gfxm::mat4& getWorld() {
        if(parent) {
            return world = parent->getWorld() * getLocal();
        } else {
            return world = getLocal();
        }
    }
};

struct ModelMesh {
    ModelNode*                  node;
    std::shared_ptr<Mesh>       mesh;
    std::shared_ptr<Material>   material;
};
struct ModelSkinCache {
    std::shared_ptr<gl::VertexArrayObject>  vao;
    std::shared_ptr<gl::Buffer>             position;
    std::shared_ptr<gl::Buffer>             normal;
    std::shared_ptr<gl::Buffer>             tangent;
    std::shared_ptr<gl::Buffer>             bitangent;
    std::shared_ptr<gl::Buffer>             pose;
};
struct ModelSkin {
    ModelMesh                   mesh;
    std::vector<ModelNode*>     bone_nodes;
    std::vector<gfxm::mat4>     bind_transforms;
    ModelSkinCache              skin_cache;
};

class Model_ : public Resource {
    RTTR_ENABLE(Resource)

public:
    Model_()
    : rootNode(new ModelNode) 
    {}

    std::unique_ptr<ModelNode>         rootNode;
    std::vector<ModelMesh>             meshes;
    std::vector<ModelSkin>             skin_meshes;

    ModelNode* findNode(const char* name) {
        return findLocalNode(rootNode.get(), name);
    }
    ModelNode* findLocalNode(ModelNode* parent, const char* name) {
        ModelNode* r = 0;
        for(auto& n : parent->children) {
            if(n->name == name) {
                r = n.get();
                break;
            } else {
                r = findLocalNode(n.get(), name);
            }
        }
        return r;
    }

    void serialize(out_stream& out) override {

    }
    bool deserialize(in_stream& in, size_t sz) override {
        return true;
    }
};
STATIC_RUN(Model_) {
    rttr::registration::class_<Model_>("Model")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}


#endif
