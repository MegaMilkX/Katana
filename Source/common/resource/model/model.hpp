#ifndef RES_MODEL_HPP
#define RES_MODEL_HPP

#include "../resource.h"
#include "../../gl/vertex_array_object.hpp"
#include "../../gl/vertex_buffer.hpp"
#include "../../gl/buffer.hpp"
#include "../mesh.hpp"
#include "../material.hpp"

#include "mesh_deformer.hpp"

#include "model_node.hpp"


struct ModelMesh {
    int                                 node;
    std::shared_ptr<Mesh>               mesh;
    std::shared_ptr<Material>           material;
    std::shared_ptr<MeshDeformerBase>   deformer;
};

class Model_ : public Resource {
    RTTR_ENABLE(Resource)

public:
    Model_() {
        nodes.push_back(ModelNode());
        nodes.back().name = "";
    }

    std::vector<ModelNode>             nodes; // 0 node is always root
    std::vector<ModelMesh>             meshes;

    const gfxm::mat4& getLocalTransform(int node) {
        auto& n = nodes[node];
        return n.local = gfxm::translate(gfxm::mat4(1.0f), n.translation) *
            gfxm::to_mat4(n.rotation) *
            gfxm::scale(gfxm::mat4(1.0f), n.scale);
    }
    const gfxm::mat4& getWorldTransform(int node) {
        auto& n = nodes[node];
        if(n.parent >= 0) {
            return n.world = getWorldTransform(n.parent) * getLocalTransform(node);
        } else {
            return n.world = getLocalTransform(node);
        }
    }

    int createNode(int parent, const std::string& name = "") {
        assert(parent >= 0);
        nodes.push_back(ModelNode());
        auto& n = nodes.back();
        n.parent = parent;
        n.name = name;
        nodes[parent].children.push_back(nodes.size() - 1);
        return nodes.size() - 1;
    }
    ModelNode* getRootNode() {
        return &nodes[0];
    }
    ModelNode* getNode(int node) {
        return &nodes[node];
    }
    int findNodeId(const char* name) {
        return findNodeId(0, name);
    }
    int findNodeId(int parent_id, const char* name) {
        int r = -1;
        for(auto& n : nodes[parent_id].children) {
            if(nodes[n].name == name) {
                r = n;
                break;
            } else {
                r = findNodeId(n, name);
                if (r != -1) {
                    break;
                }
            }
        }
        return r;
    }
    ModelNode* findNode(const char* name) {
        return findNode(0, name);
    }
    ModelNode* findNode(int parent_node, const char* name) {
        ModelNode* r = 0;
        for(auto& n : nodes[parent_node].children) {
            if(nodes[n].name == name) {
                r = &nodes[n];
                break;
            } else {
                r = findNode(n, name);
                if(r) {
                    break;
                }
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


#include "../../draw_list.hpp"
#include "../../mesh_pool.hpp"

inline void modelToDrawList(DrawList& dl, Model_* model, const gfxm::mat4& transform, void* pick_id, int group_mask) {
    auto cube_mesh = MeshPool::get(PRIM_CUBE);
    if(!model) {
        DrawCmdSolid cmd;
        cmd.vao = cube_mesh->getVao();
        cmd.material = 0;
        cmd.indexCount = cube_mesh->getIndexCount();
        cmd.indexOffset = 0;
        cmd.transform = transform;
        cmd.object_ptr = pick_id;
        cmd.lightmap = 0;
        dl.solids.emplace_back(cmd);
    } else {
        for(int i = 0; i < model->meshes.size(); ++i) {
            DrawCmdSolid cmd;
            const auto& m = model->meshes[i];
            auto draw_cmd_id = dl.solids.size();

            if(m.deformer) {
                dl.deformers[m.deformer->getType()].push_back(DrawList::DeformerToCmd{m.deformer.get(), draw_cmd_id});
            }

            cmd.vao = m.mesh->mesh.getVao();
            cmd.material = m.material.get();
            cmd.indexCount = m.mesh->mesh.getIndexCount();
            cmd.indexOffset = 0;
            cmd.transform = transform * model->getWorldTransform(m.node);
            cmd.object_ptr = pick_id;
            cmd.lightmap = 0;

            dl.solids.emplace_back(cmd);
            for(int i = 0; i < DRAW_GROUP_COUNT; ++i) {
                if(group_mask & (1 << i)) {
                    dl.draw_groups[i].emplace_back(dl.solids.size() - 1);
                }
            }
        }
    }
}

#endif
