#ifndef ECS_RENDER_SYSTEM_HPP
#define ECS_RENDER_SYSTEM_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"


class ecsRenderSubSystem;
class ecsTupleSubSceneRenderable : public ecsTuple<ecsWorldTransform, ecsSubScene, ecsTagSubSceneRender> {
public:
    ecsRenderSubSystem* sub_system = 0;

    void onAttribUpdate(ecsSubScene* scn) override {
        if(!scn->getWorld()) {
            sub_system = 0;
        } else {
            sub_system = scn->getWorld()->getSystem<ecsRenderSubSystem>();
        }
    }
};

class ecsRenderSubSystem : public ecsSystem<
    ecsTuple<ecsWorldTransform, ecsMeshes>,
    ecsTuple<ecsWorldTransform, ecsLightOmni>,
    ecsTupleSubSceneRenderable
> {
public:
    void fillDrawList(DrawList& dl, const gfxm::mat4& root_transform, entity_id subscene_owner) {
        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsLightOmni>>()) {
            dl.omnis.emplace_back(
                DrawList::OmniLight {
                    root_transform * gfxm::vec4(0,0,0,1),
                    a->get<ecsLightOmni>()->color,
                    a->get<ecsLightOmni>()->intensity,
                    a->get<ecsLightOmni>()->radius
                }
            );
        }
        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>()) {
            for(auto& seg : a->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = a->get<ecsWorldTransform>()->transform;
                size_t indexOffset = seg.mesh->submeshes.size() > 0 ? seg.mesh->submeshes[seg.submesh_index].indexOffset : 0;
                size_t indexCount = seg.mesh->submeshes.size() > 0 ? (seg.mesh->submeshes[seg.submesh_index].indexCount) : seg.mesh->mesh.getIndexCount();
                
                if(!seg.skin_data) {    // Static mesh
                    DrawCmdSolid s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = root_transform * transform;
                    s.object_ptr = (void*)subscene_owner;
                    s.lightmap = seg.lightmap.get();
                    dl.solids.emplace_back(s);
                } else {                // Skinned mesh
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : seg.skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(root_transform * t->transform);
                        } else {
                            bone_transforms.emplace_back(root_transform * gfxm::mat4(1.0f));
                        }
                    }

                    DrawCmdSkin s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = root_transform * transform;
                    s.bone_transforms = bone_transforms;
                    s.bind_transforms = seg.skin_data->bind_transforms;
                    s.object_ptr = (void*)subscene_owner;
                    dl.skins.emplace_back(s);
                }
            }
        }
    }
};

class ecsRenderSystem : public ecsSystem<
    ecsTuple<ecsWorldTransform, ecsMeshes>,
    ecsTuple<ecsWorldTransform, ecsLightOmni>,
    ecsTupleSubSceneRenderable
> {
    DrawList draw_list;
public:
    ecsRenderSystem() {}

    void onFit(ecsTupleSubSceneRenderable* r) {
        if(r->get<ecsSubScene>()->getWorld()) {
            ecsRenderSubSystem* sys = r->get<ecsSubScene>()->getWorld()->getSystem<ecsRenderSubSystem>();
            r->sub_system = sys;
        }
    }

    void fillDrawList(DrawList& dl) {
        for(auto& a : get_array<ecsTupleSubSceneRenderable>()) {
            a->sub_system->fillDrawList(dl, a->get<ecsWorldTransform>()->transform, a->getEntityUid());
        }

        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsLightOmni>>()) {
            dl.omnis.emplace_back(
                DrawList::OmniLight {
                    a->get<ecsWorldTransform>()->transform * gfxm::vec4(0,0,0,1),
                    a->get<ecsLightOmni>()->color,
                    a->get<ecsLightOmni>()->intensity,
                    a->get<ecsLightOmni>()->radius
                }
            );
        }

        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>()) {
            for(auto& seg : a->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = a->get<ecsWorldTransform>()->transform;
                size_t indexOffset = seg.mesh->submeshes.size() > 0 ? seg.mesh->submeshes[seg.submesh_index].indexOffset : 0;
                size_t indexCount = seg.mesh->submeshes.size() > 0 ? (seg.mesh->submeshes[seg.submesh_index].indexCount) : seg.mesh->mesh.getIndexCount();
                
                if(!seg.skin_data) {    // Static mesh
                    DrawCmdSolid s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = transform;
                    s.object_ptr = (void*)a->getEntityUid();
                    s.lightmap = seg.lightmap.get();
                    dl.solids.emplace_back(s);
                } else {                // Skinned mesh
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : seg.skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(t->transform);
                        } else {
                            bone_transforms.emplace_back(gfxm::mat4(1.0f));
                        }
                    }

                    DrawCmdSkin s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = transform;
                    s.bone_transforms = bone_transforms;
                    s.bind_transforms = seg.skin_data->bind_transforms;
                    s.object_ptr = (void*)a->getEntityUid();
                    dl.skins.emplace_back(s);
                }
            }
        }
    }

    void fillDrawList(DrawList& dl, entity_id entity_filter) {
        for(auto& a : get_array<ecsTupleSubSceneRenderable>()) {
            if(a->getEntityUid() == entity_filter) {
                a->sub_system->fillDrawList(dl, a->get<ecsWorldTransform>()->transform, a->getEntityUid());
                break;
            }
        }

        for(auto& a : get_array<ecsTuple<ecsWorldTransform, ecsMeshes>>()) {
            if(a->getEntityUid() != entity_filter) {
                continue;
            }
            for(auto& seg : a->get<ecsMeshes>()->segments) {
                if(!seg.mesh) continue;
                
                GLuint vao = seg.mesh->mesh.getVao();
                Material* mat = seg.material.get();
                gfxm::mat4 transform = a->get<ecsWorldTransform>()->transform;
                size_t indexOffset = seg.mesh->submeshes.size() > 0 ? seg.mesh->submeshes[seg.submesh_index].indexOffset : 0;
                size_t indexCount = seg.mesh->submeshes.size() > 0 ? (seg.mesh->submeshes[seg.submesh_index].indexCount) : seg.mesh->mesh.getIndexCount();
                
                if(!seg.skin_data) {    // Static mesh
                    DrawCmdSolid s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = transform;
                    //s.object_ptr = getOwner();
                    dl.solids.emplace_back(s);
                } else {                // Skinned mesh
                    std::vector<gfxm::mat4> bone_transforms;
                    for(auto t : seg.skin_data->bone_nodes) {
                        if(t) {
                            bone_transforms.emplace_back(t->transform);
                        } else {
                            bone_transforms.emplace_back(gfxm::mat4(1.0f));
                        }
                    }

                    DrawCmdSkin s;
                    s.vao = vao;
                    s.material = mat;
                    s.indexCount = indexCount;
                    s.indexOffset = indexOffset;
                    s.transform = transform;
                    s.bone_transforms = bone_transforms;
                    s.bind_transforms = seg.skin_data->bind_transforms;
                    //s.object_ptr = getOwner();
                    dl.skins.emplace_back(s);
                }
            }
        }
    }

};


#endif
