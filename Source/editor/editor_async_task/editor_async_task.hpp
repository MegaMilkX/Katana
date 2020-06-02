#ifndef EDITOR_ASYNC_TASK_HPP
#define EDITOR_ASYNC_TASK_HPP

#include "../util/editor_task_mgr.hpp"
#include "../../common/ecs/world.hpp"
#include "../../common/ecs/attribs/base_attribs.hpp"
#include "../../common/gui_viewport.hpp"
#include "../../common/ecs/util/assimp_to_ecs_world.hpp"
#include "../util/lightmap/gen_lightmaps.hpp"


class edTaskEcsWorldModelDragNDrop : public edTaskAsync {
public:
    std::string entity_name;
    std::string file_path;
    ecsSubScene subscene;
    ecsWorld*   target_world;

    edTaskEcsWorldModelDragNDrop(const char* name, const char* file_path, const char* entity_name, ecsWorld* target_world)
        : edTaskAsync(name), file_path(file_path), entity_name(entity_name), target_world(target_world)
    {
        
    }

    void execute() override {
        subscene = ecsSubScene(std::shared_ptr<ecsWorld>(new ecsWorld()));
        ecsysSceneGraph* sceneGraph = subscene.getWorld()->getSystem<ecsysSceneGraph>();
        assimpImportEcsScene(sceneGraph, file_path.c_str());
    }
};

class edTaskEcsWorldCalcLightmaps : public edTaskAsync {
public:
    ecsRenderSystem* renderSys = 0;
    GuiViewport* gvp = 0; // Ugh
    DrawList dl;

    edTaskEcsWorldCalcLightmaps(const char* name)
        : edTaskAsync(name)
    {
        
    }

    void execute() override {
        auto entities = renderSys->get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>();
        std::vector<LightmapMeshData> mesh_data;
        delegatedCall([&entities, &mesh_data](){
            for(auto& e : entities) {
                auto m = e->get<ecsMeshes>();
                for(auto& seg : m->segments) {
                    if(!seg.mesh) {
                        continue;
                    }
                    if(!seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)
                        || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Normal)
                        || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Position)
                    ) {
                        continue;
                    }

                    mesh_data.resize(mesh_data.size() + 1);
                    auto& md = mesh_data.back();

                    md.transform = e->get<ecsWorldTransform>()->transform;
                    md.tex_width = 256;
                    md.tex_height = 256;
                    md.position.resize(seg.mesh->vertexCount() * 3);
                    seg.mesh->mesh.copyAttribData(VERTEX_FMT::ENUM_GENERIC::Position, md.position.data());
                    md.normal.resize(seg.mesh->vertexCount() * 3);
                    seg.mesh->mesh.copyAttribData(VERTEX_FMT::ENUM_GENERIC::Normal, md.normal.data());
                    md.uv_lightmap.resize(seg.mesh->vertexCount() * 2);
                    seg.mesh->mesh.copyAttribData(VERTEX_FMT::ENUM_GENERIC::UVLightmap, md.uv_lightmap.data());
                    md.indices.resize(seg.mesh->indexCount());
                    seg.mesh->mesh.copyIndexData(md.indices.data());
                    md.segment = &seg;
                }
            }
        });

        // TODO: USE SEPARATE GBUFFER WTF
        GenLightmaps(mesh_data, gvp->getRenderer(), gvp->getViewport()->getGBuffer(), dl);

        int i = 0;
        for(auto& e : entities) {
            auto m = e->get<ecsMeshes>();
            for(auto& seg : m->segments) {
                if(!seg.mesh) {
                    continue;
                }
                if(!seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)
                    || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Normal)
                    || !seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Position)
                ) {
                    continue;
                }

                seg.lightmap = mesh_data[i].lightmap;
                
                ++i;
            }
        }
    }
};


#endif
