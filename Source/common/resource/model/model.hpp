#ifndef RES_MODEL_HPP
#define RES_MODEL_HPP

#include "../resource.h"

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

class Model_ : public Resource {
    RTTR_ENABLE(Resource)
public:
    struct MeshLink {
        Mesh*       mesh = 0;
        ModelNode*  node = 0;
    };
private:

public:
    Model_()
    : rootNode(new ModelNode) 
    {}

    std::unique_ptr<ModelNode>         rootNode;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<MeshLink>              meshLinks;

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
