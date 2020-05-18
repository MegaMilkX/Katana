#ifndef ECS_BASE_ATTRIBS_HPP
#define ECS_BASE_ATTRIBS_HPP

#include "../common/resource/mesh.hpp"
#include "../common/util/imgui_helpers.hpp"
#include <btBulletCollisionCommon.h>


#include "../common/ecs/attribs/transform.hpp"
#include "../common/ecs/attribs/transform_tree.hpp"

#include "../common/ecs/attribs/scene_graph_attribs.hpp"

#include "kt_cmd.hpp"

#include "../../util/threading/delegated_call.hpp"

class ecsSubScene : public ecsAttrib<ecsSubScene> {
    std::shared_ptr<ecsWorld> world;
public:
    ecsSubScene()
    : world(new ecsWorld()) {}
    ecsSubScene(std::shared_ptr<ecsWorld> world)
    : world(world) {}
    ecsWorld* getWorld() const { return world.get(); }
    void onGui(ecsWorld* world, entity_id ent) override {
        if(ImGui::Button(ICON_MDI_PENCIL " Edit", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
            kt_cmd(MKSTR("ecs_world_subdoc " << (uint64_t)this->world.get()).c_str());
        }
    }

    ecsWorld* getWorld() {
        return world.get();
    }

    void write(ecsWorldWriteCtx& out) override {
        world->serialize(*out.getStream());
    }
    void read(ecsWorldReadCtx& in) override {
        world.reset(new ecsWorld());
        world->deserialize(*in.getStream(), in.getStream()->bytes_available());
    }
};

class ecsTagSubSceneRender : public ecsAttrib<ecsTagSubSceneRender> {};

class ecsName : public ecsAttrib<ecsName> {
public:
    std::string name;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        char buf[256];
        memset(buf, 0, sizeof(buf));
        memcpy(buf, name.c_str(), name.size());
        if(ImGui::InputText("name", buf, sizeof(buf))) {
            name = buf;
        }
    }

    void write(ecsWorldWriteCtx& out) override {
        out.writeStr(name);
    }
    void read(ecsWorldReadCtx& in) override {
        name = in.readStr();
    }
};

class ecsVelocity : public ecsAttrib<ecsVelocity> {
public:
    gfxm::vec3 velo;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        ImGui::DragFloat3("velo", (float*)&velo, 0.01f);
    }

    void write(ecsWorldWriteCtx& out) override {
        out.write(velo);
    }
    void read(ecsWorldReadCtx& in) override {
        velo = in.read<gfxm::vec3>();
    }
};
class ecsMass : public ecsAttrib<ecsMass> {
public:
    float mass = 1.0f;
    virtual void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::DragFloat("mass", &mass, 0.01f)) {
            world->updateAttrib(ent, *this);
        }
    }

    void write(ecsWorldWriteCtx& out) override {
        out.write(mass);
    }
    void read(ecsWorldReadCtx& in) override {
        mass = in.read<float>();
    }
};


#include "../../attributes/collision_shapes.hpp"
class ecsCollisionShape : public ecsAttrib<ecsCollisionShape> {
public:
    ecsCollisionShape() {
        shape.reset(new btSphereShape(.5f));
    }
    std::shared_ptr<btCollisionShape> shape;

    void write(ecsWorldWriteCtx& out) override {
        
    }
    void read(ecsWorldReadCtx& in) override {
        
    }
};

#include "../../util/geom/gen_lightmap_uv.hpp"
#include "../../gl/buffer.hpp"
#include "../../gl/vertex_array_object.hpp"
class ecsMeshes : public ecsAttrib<ecsMeshes> {
public:
    struct SkinData {
        std::vector<ecsWorldTransform*>         bone_nodes;
        std::vector<gfxm::mat4>                 bind_transforms;
        std::shared_ptr<gl::VertexArrayObject>  vao_cache;
        std::shared_ptr<gl::Buffer>             position_cache;
        std::shared_ptr<gl::Buffer>             normal_cache;
        std::shared_ptr<gl::Buffer>             tangent_cache;
        std::shared_ptr<gl::Buffer>             bitangent_cache;
        std::shared_ptr<gl::Buffer>             pose_cache;
    };
    struct Segment {
        std::shared_ptr<Mesh>       mesh;
        uint8_t                     submesh_index;
        std::shared_ptr<Material>   material;
        std::shared_ptr<SkinData>   skin_data;
        std::shared_ptr<Texture2D>  lightmap;
    };

    std::vector<Segment> segments;

    Segment& getSegment(size_t i) {
        if(i >= segments.size()) {
            segments.resize(i + 1);
        }
        return segments[i];
    }
    void removeSegment(size_t i) {
        segments.erase(segments.begin() + i);
    }
    size_t segmentCount() const {
        return segments.size();
    }

    virtual void onGui(ecsWorld* world, entity_id ent) {
        if(ImGui::Button("Gen Lightmap UV Layer", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
            std::set<Mesh*> unique_meshes;
            for(auto& seg : segments) {
                unique_meshes.insert(seg.mesh.get());
            }

            for(auto& m : unique_meshes) {
                GenLightmapUV(m);
            }
        }
        
        for(size_t i = 0; i < segmentCount(); ++i) {
            auto& seg = getSegment(i);

            ImGuiTreeNodeFlags tree_node_flags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;
            bool open = ImGui::TreeNodeEx(MKSTR("Mesh segment " << i).c_str(), tree_node_flags);
            if(open) {
                ImGui::Text(MKSTR("Segment " << i).c_str());
                bool seg_removed = false;
                ImGui::SameLine();
                if(ImGui::SmallButton(ICON_MDI_DELETE)) {
                    seg_removed = true;
                }
                imguiResourceTreeCombo(MKSTR("mesh##" << i).c_str(), seg.mesh, "msh", [this](){
                    LOG("Mesh changed");
                });
                if(seg.mesh && (seg.mesh->submeshes.size() > 1)) {
                    int submesh_index = (int)seg.submesh_index;
                    if(ImGui::DragInt(MKSTR("submesh##" << i).c_str(), &submesh_index, 1.0f, 0, seg.mesh->submeshes.size() - 1)) {
                        seg.submesh_index = (uint8_t)submesh_index;
                    }
                }
                imguiResourceTreeCombo(MKSTR("material##" << i).c_str(), seg.material, "mat", [this](){
                    LOG("Material changed");
                });
                if(seg.lightmap) {
                    ImGui::Image(
                        (ImTextureID)seg.lightmap->GetGlName(), ImVec2(100, 100), 
                        ImVec2(0, 1), ImVec2(1, 0)
                    );
                }
                imguiResourceTreeCombo(MKSTR("lightmap##" << i).c_str(), seg.lightmap, "png", [this](){
                    LOG("lightmap changed");
                });
                if(seg.mesh) {
                    ImGui::Text("Attributes:");
                    for(int j = 0; j < VERTEX_FMT::GENERIC::attribCount(); ++j) {
                        if(seg.mesh->mesh.getAttribBuffer(j)) {
                            ImGui::Text(MKSTR(VERTEX_FMT::GENERIC::getAttribDesc(j).name).c_str());
                        }
                    }
                }
                if(seg_removed) {
                    removeSegment(i);
                }
                
                ImGui::TreePop();
            }
        }
        if(ImGui::Button(ICON_MDI_PLUS " Add segment")) {
            getSegment(segmentCount());
        }
    }

    void write(ecsWorldWriteCtx& w) override {

        w.write<uint32_t>(segments.size());
        for(size_t i = 0; i < segments.size(); ++i) {
            std::string mesh_name = "";
            std::string mat_name = "";

            if(getSegment(i).mesh) mesh_name = getSegment(i).mesh->Name();
            if(getSegment(i).material) mat_name = getSegment(i).material->Name();

            w.writeResource(getSegment(i).mesh);
            w.write<uint8_t>(getSegment(i).submesh_index);
            w.writeResource(getSegment(i).material);

            if(getSegment(i).skin_data) {
                auto& skin_data = getSegment(i).skin_data;
                w.write<uint32_t>(skin_data->bone_nodes.size());
                for(size_t j = 0; j < skin_data->bone_nodes.size(); ++j) {
                    w.writeAttribRef(skin_data->bone_nodes[j]);
                    w.write<gfxm::mat4>(skin_data->bind_transforms[j]);
                }
            } else {
                w.write<uint32_t>(0);
            }
        }
    }
    void read(ecsWorldReadCtx& r) override {
        uint32_t seg_count = r.read<uint32_t>();
        for(uint32_t i = 0; i < seg_count; ++i) {
            auto& seg = getSegment(i);
            seg.mesh = r.readResource<Mesh>();
            seg.submesh_index = (uint8_t)r.read<uint8_t>();
            seg.material = r.readResource<Material>();

            uint32_t bone_count = r.read<uint32_t>();
            delegatedCall([this, bone_count, &seg](){
                if(bone_count && seg.mesh) {
                    seg.skin_data.reset(new SkinData());

                    // TODO: Move this to a separate function
                    size_t vertexCount = seg.mesh->vertexCount();
                    seg.skin_data->vao_cache.reset(new gl::VertexArrayObject);
                    seg.skin_data->position_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                    seg.skin_data->normal_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                    seg.skin_data->tangent_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                    seg.skin_data->bitangent_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                    seg.skin_data->vao_cache->attach(seg.skin_data->position_cache->getId(), 
                        VERTEX_FMT::ENUM_GENERIC::Position, VERTEX_FMT::Position::count, 
                        VERTEX_FMT::Position::gl_type, VERTEX_FMT::Position::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->vao_cache->attach(seg.skin_data->normal_cache->getId(), 
                        VERTEX_FMT::ENUM_GENERIC::Normal, VERTEX_FMT::Normal::count, 
                        VERTEX_FMT::Normal::gl_type, VERTEX_FMT::Normal::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->vao_cache->attach(seg.skin_data->tangent_cache->getId(), 
                        VERTEX_FMT::ENUM_GENERIC::Tangent, VERTEX_FMT::Tangent::count, 
                        VERTEX_FMT::Tangent::gl_type, VERTEX_FMT::Tangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->vao_cache->attach(seg.skin_data->bitangent_cache->getId(), 
                        VERTEX_FMT::ENUM_GENERIC::Bitangent, VERTEX_FMT::Bitangent::count, 
                        VERTEX_FMT::Bitangent::gl_type, VERTEX_FMT::Bitangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->pose_cache.reset(new gl::Buffer(GL_STREAM_DRAW, sizeof(float) * 16 * bone_count));

                    if(seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UV)) {
                        GLuint id = seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UV)->getId();
                        seg.skin_data->vao_cache->attach(id, VERTEX_FMT::ENUM_GENERIC::UV, 
                            VERTEX_FMT::UV::count, VERTEX_FMT::UV::gl_type, 
                            VERTEX_FMT::UV::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                    }
                    if(seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)) {
                        GLuint id = seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)->getId();
                        seg.skin_data->vao_cache->attach(id, VERTEX_FMT::ENUM_GENERIC::UVLightmap, 
                            VERTEX_FMT::UVLightmap::count, VERTEX_FMT::UVLightmap::gl_type, 
                            VERTEX_FMT::UVLightmap::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                    }
                    if(seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::ColorRGBA)) {
                        GLuint id = seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::ColorRGBA)->getId();
                        seg.skin_data->vao_cache->attach(id, VERTEX_FMT::ENUM_GENERIC::ColorRGBA, 
                            VERTEX_FMT::ColorRGBA::count, VERTEX_FMT::ColorRGBA::gl_type, 
                            VERTEX_FMT::ColorRGBA::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                    }
                    GLuint index_buf_id = seg.mesh->mesh.getIndexBuffer()->getId();
                    seg.skin_data->vao_cache->attachIndexBuffer(index_buf_id);
                }
            });

            if(bone_count && seg.mesh) {
                for(uint32_t j = 0; j < bone_count; ++j) {
                    ecsWorldTransform* attr = (ecsWorldTransform*)r.readAttribRef();
                    gfxm::mat4 m = r.read<gfxm::mat4>();
                    seg.skin_data->bone_nodes.emplace_back(attr);
                    seg.skin_data->bind_transforms.emplace_back(m);
                }
            }
        }
    }
};

class ecsLightOmni : public ecsAttrib<ecsLightOmni> {
public:
    gfxm::vec3 color = gfxm::vec3(1,1,1);
    float      radius = 1.0f;
    float      intensity = 1.0f;

    void onGui(ecsWorld* world, entity_id ent) override {
        ImGui::ColorEdit3(MKSTR("color##" << this).c_str(), (float*)&color);
        ImGui::DragFloat(MKSTR("radius##" << this).c_str(), &radius, 0.01f);
        ImGui::DragFloat(MKSTR("intensity##" << this).c_str(), &intensity, 0.01f);
    }

    void write(ecsWorldWriteCtx& w) override {
        w.write(color);
        w.write(radius);
        w.write(intensity);
    }
    void read(ecsWorldReadCtx& r) override {
        color = r.read<gfxm::vec3>();
        radius = r.read<float>();
        intensity = r.read<float>();
    }
};


#endif
