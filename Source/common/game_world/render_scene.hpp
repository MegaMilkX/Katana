#ifndef KT_RENDER_WORLD_HPP
#define KT_RENDER_WORLD_HPP

#include "../gfxm.hpp"
#include "../resource/mesh.hpp"
#include "../resource/material.hpp"
#include "../draw_list.hpp"
#include "../util/octree.hpp"

#include "game_object/node/node.hpp"

#include <set>
#include <debug_draw.hpp>
#include <draw_list.hpp>

#include "../mesh_pool.hpp"

#include <game_world/transform.hpp>

struct rsMesh {
    hTransform  transform;
    std::shared_ptr<Mesh> mesh;
    gfxm::aabb  aabb;
    int32_t     octree_object;
};
struct rsCamera {
    gfxm::mat4 view;
    gfxm::mat4 projection;
    float fov = gfxm::radian(90.0f);
    float znear = 0.01f;
    float zfar  = 1000.0f;
};
struct rsLightOmni {
    hTransform transform;
    gfxm::vec3 color;
    float intensity;
    float radius;
};

struct RenderMeshData : public ktDirtyArrayElement {
    struct elem {
        Mesh*       mesh;
        Material*   material;
        hTransform  transform;
        gfxm::aabb  aabb;
    };

    gfxm::aabb                  aabb;
    int32_t                     octree_index;
    std::vector<elem>           elements;
};

class ktRenderScene {
    Octree octree;

    std::set<ktNodeRenderable*> renderables;

    // ***
    std::set<RenderMeshData*> render_datas;
    std::set<rsLightOmni*> lights_omni;

    ktDirtyArray<RenderMeshData> dirty_array;
    // ***

public:
    void addRenderData(RenderMeshData* rd) {
        assert(rd->elements.size() > 0);
        rd->octree_index = octree.createObject(rd->aabb, (void*)rd);

        for(auto& e : rd->elements) {
            dirty_array.add(rd);
            e.transform->linkDirtyArray(&dirty_array, rd);
        }

        // Temporary
        render_datas.insert(rd);
    }
    void removeRenderData(RenderMeshData* rd) {
        render_datas.erase(rd);

        for(auto& e : rd->elements) {
            e.transform->unlinkDirtyArray(&dirty_array, rd);
            dirty_array.remove(rd);
        }

        octree.deleteObject(rd->octree_index);
    }
    void addLightOmni(rsLightOmni* omni) {
        lights_omni.insert(omni);
    }
    void removeLightOmni(rsLightOmni* omni) {
        lights_omni.erase(omni);
    }

    void fillDrawList(rsCamera* cam, DrawList* dl) {
        for(int i = 0; i < dirty_array.dirtyCount(); ++i) {
            auto render_data = dirty_array[i];
            gfxm::aabb combined_aabb;
            if(render_data->elements.size() > 0) {
                combined_aabb = gfxm::aabb_transform(render_data->elements[0].aabb, render_data->elements[0].transform->getWorldTransform());
            }
            for(int j = 1; j < render_data->elements.size(); ++j) {
                auto& e = render_data->elements[j];
                gfxm::aabb box = gfxm::aabb_transform(e.aabb, e.transform->getWorldTransform());
                combined_aabb.from.x = gfxm::_min(combined_aabb.from.x, box.from.x);
                combined_aabb.from.y = gfxm::_min(combined_aabb.from.y, box.from.y);
                combined_aabb.from.z = gfxm::_min(combined_aabb.from.z, box.from.z);
                combined_aabb.to.x = gfxm::_max(combined_aabb.to.x, box.to.x);
                combined_aabb.to.y = gfxm::_max(combined_aabb.to.y, box.to.y);
                combined_aabb.to.z = gfxm::_max(combined_aabb.to.z, box.to.z); 
            }
            octree.updateObject(render_data->octree_index, combined_aabb);
        }
        dirty_array.clearDirtyCount();

        auto cubeMesh = MeshPool::get(PRIMITIVE_MESH::PRIM_CUBE);

        gfxm::frustum fr = gfxm::make_frustum(cam->projection, cam->view);
        auto cell_list = octree.listVisibleCells(fr);
        for(auto cell : cell_list) {
            for(auto o : octree.getCell(cell)->objects) {
                RenderMeshData* rd = (RenderMeshData*)octree.getObject(o)->user_ptr;

                for(auto& e : rd->elements) {
                    gl::IndexedMesh* pMesh = e.mesh ? &e.mesh->mesh : cubeMesh;
                    DrawCmdSolid cmd;
                    cmd.indexCount = pMesh->getIndexCount();
                    cmd.indexOffset = 0;
                    cmd.lightmap = 0;
                    cmd.material = 0;
                    cmd.transform = e.transform ? e.transform->getWorldTransform() : gfxm::mat4(1.0f);
                    cmd.vao = pMesh->getVao();
                    dl->solids.push_back(cmd);
                }
                /*
                gl::IndexedMesh* pMesh = m->mesh ? &m->mesh->mesh : cubeMesh;
                DrawCmdSolid cmd;
                cmd.indexCount = pMesh->getIndexCount();
                cmd.indexOffset = 0;
                cmd.lightmap = 0;
                cmd.material = 0;
                cmd.transform = m->transform ? m->transform->getWorldTransform() : gfxm::mat4(1.0f);
                cmd.vao = pMesh->getVao();
                dl->solids.push_back(cmd);*/
            }
        }

        for(auto l : lights_omni) {
            DrawList::OmniLight light;
            light.translation = l->transform->getWorldTransform() * gfxm::vec4(0,0,0,1);
            light.radius = l->radius;
            light.intensity = l->intensity;
            light.color = l->color;
            dl->omnis.push_back(light);
        }
    }


    void addRenderable(ktNodeRenderable* renderable) {
        renderable->getRenderData()->octree_object = octree.createObject(renderable->getRenderData()->aabb, renderable->getRenderData());
        renderables.insert(renderable);
    }
    void removeRenderable(ktNodeRenderable* renderable) {
        renderables.erase(renderable);
        octree.deleteObject(renderable->getRenderData()->octree_object);
    }

    void update() {
        for(auto r : renderables) {
            auto transform = r->getWorldTransform();
            auto rd = r->getRenderData();

            rd->aabb_transformed = gfxm::aabb_transform(rd->aabb, transform);
            octree.updateObject(rd->octree_object, rd->aabb_transformed);
            rd->cmd.transform = transform;
        }
    }

    void updateDrawList(const gfxm::frustum& frustum, DrawList& dl) {
        for(auto rd : renderables) {
            dl.solids.push_back(rd->getRenderData()->cmd);
        }
    }

    void debugDraw(DebugDraw* dd) {
        octree.debugDraw(*dd);

        for(auto r : renderables) {
            dd->aabb(r->getRenderData()->aabb_transformed, gfxm::vec3(0.6f, 0.6f, 0.6f));
        }
    }
};

#endif
