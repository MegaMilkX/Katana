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

struct rsMesh {
    gfxm::mat4  transform;
    gfxm::aabb  aabb;
    int32_t     octree_object;
};
class rsModel {
public:
    std::vector<std::unique_ptr<Mesh>> meshes;
};
struct rsCamera {
    gfxm::mat4 view;
    gfxm::mat4 projection;
    float fov = gfxm::radian(90.0f);
    float znear = 0.01f;
    float zfar  = 1000.0f;
};

class ktRenderScene {
    Octree octree;

    std::set<ktNodeRenderable*> renderables;

    gfxm::aabb transformAABB(const gfxm::aabb& box, const gfxm::mat4& transform) {
        gfxm::vec3 points[8] = {
            box.from,
            gfxm::vec3(box.from.x, box.from.y, box.to.z),
            gfxm::vec3(box.from.x, box.to.y, box.from.z),
            gfxm::vec3(box.to.x, box.from.y, box.from.z),
            box.to,
            gfxm::vec3(box.to.x, box.to.y, box.from.z),
            gfxm::vec3(box.to.x, box.from.y, box.to.z),
            gfxm::vec3(box.from.x, box.to.y, box.to.z)
        };
        for(int i = 0; i < 8; ++i) {
            points[i] = transform * gfxm::vec4(points[i], 1.0f);
        }
        gfxm::aabb aabb;
        aabb.from = points[0];
        aabb.to   = points[0];
        for(int i = 1; i < 8; ++i) {
            aabb.from = gfxm::vec3(
                gfxm::_min(aabb.from.x, points[i].x),
                gfxm::_min(aabb.from.y, points[i].y),
                gfxm::_min(aabb.from.z, points[i].z)
            );
            aabb.to = gfxm::vec3(
                gfxm::_max(aabb.to.x, points[i].x),
                gfxm::_max(aabb.to.y, points[i].y),
                gfxm::_max(aabb.to.z, points[i].z)
            );
        }
        return aabb;
    }

public:
    void addMesh(rsMesh* mesh) {
        mesh->octree_object = octree.createObject(mesh->aabb, 0);
    }
    void removeMesh(rsMesh* mesh) {
        octree.deleteObject(mesh->octree_object);
    }
    void fillDrawList(rsCamera* cam, DrawList* dl) {
        auto mesh = MeshPool::get(PRIMITIVE_MESH::PRIM_CUBE);
        DrawCmdSolid cmd;
        cmd.indexCount = mesh->getIndexCount();
        cmd.indexOffset = 0;
        cmd.lightmap = 0;
        cmd.material = 0;
        cmd.transform = gfxm::mat4(1.0f);
        cmd.vao = mesh->getVao();
        dl->solids.push_back(cmd);
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

            rd->aabb_transformed = transformAABB(rd->aabb, transform);
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
