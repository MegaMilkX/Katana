#ifndef ECS_BASE_ATTRIBS_HPP
#define ECS_BASE_ATTRIBS_HPP

#include "../common/resource/mesh.hpp"
#include "../common/util/imgui_helpers.hpp"
#include <btBulletCollisionCommon.h>


#include "../common/ecs/attribs/transform.hpp"
#include "../common/ecs/attribs/transform_tree.hpp"

#include "../common/ecs/attribs/scene_graph_attribs.hpp"
#include "gui_element.hpp"

#include "kt_cmd.hpp"

#include "../../util/threading/delegated_call.hpp"

#include "model.hpp"
#include "animator.hpp"
#include "behavior.hpp"

#include "system_attribs.hpp"

class ecsSubScene : public ecsAttrib<ecsSubScene> {
    std::shared_ptr<ecsWorld> world;
public:
    ecsSubScene()
    : world(new ecsWorld()) {
        world->setParentWorldEntity(getEntityHdl());
    }
    ecsSubScene(std::shared_ptr<ecsWorld> world)
    : world(world) {
        world->setParentWorldEntity(getEntityHdl());
    }
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
        world->setParentWorldEntity(getEntityHdl());
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


class ecsConstraint : public ecsAttrib<ecsConstraint> {
    ecsEntityHandle target_hdl;

public:
    void setTarget(ecsEntityHandle hdl) {
        target_hdl = hdl;
    }
    ecsEntityHandle& getTarget() {
        return target_hdl;
    }

    void onGui(ecsWorld* world, entity_id ent) override {
        if(imguiEntityCombo("target", world, target_hdl)) {

        }
    }

    void write(ecsWorldWriteCtx& out) override {
        
    }
    void read(ecsWorldReadCtx& in) override {
        
    }

};


class ecsIK : public ecsAttrib<ecsIK> {
public:
    
};


#include "../../util/geom/gen_lightmap_uv.hpp"
#include "../../gl/buffer.hpp"
#include "../../gl/vertex_array_object.hpp"
class ecsMeshes : public ecsAttrib<ecsMeshes> {
public:
    struct SkinData {
        std::vector<ecsEntityHandle>            bone_nodes;
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
                    for(int j = 0; j < VFMT::GENERIC::attribCount(); ++j) {
                        if(seg.mesh->mesh.getAttribBuffer(j)) {
                            ImGui::Text(MKSTR(VFMT::GENERIC::getAttribDesc(j).name).c_str());
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
                    w.writeAttribRef(skin_data->bone_nodes[j].getId(), ecsWorldTransform::get_id_static());
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
                        VFMT::ENUM_GENERIC::Position, VFMT::Position::count, 
                        VFMT::Position::gl_type, VFMT::Position::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->vao_cache->attach(seg.skin_data->normal_cache->getId(), 
                        VFMT::ENUM_GENERIC::Normal, VFMT::Normal::count, 
                        VFMT::Normal::gl_type, VFMT::Normal::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->vao_cache->attach(seg.skin_data->tangent_cache->getId(), 
                        VFMT::ENUM_GENERIC::Tangent, VFMT::Tangent::count, 
                        VFMT::Tangent::gl_type, VFMT::Tangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->vao_cache->attach(seg.skin_data->bitangent_cache->getId(), 
                        VFMT::ENUM_GENERIC::Bitangent, VFMT::Bitangent::count, 
                        VFMT::Bitangent::gl_type, VFMT::Bitangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                    );
                    seg.skin_data->pose_cache.reset(new gl::Buffer(GL_STREAM_DRAW, sizeof(float) * 16 * bone_count));

                    if(seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UV)) {
                        GLuint id = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UV)->getId();
                        seg.skin_data->vao_cache->attach(id, VFMT::ENUM_GENERIC::UV, 
                            VFMT::UV::count, VFMT::UV::gl_type, 
                            VFMT::UV::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                    }
                    if(seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UVLightmap)) {
                        GLuint id = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UVLightmap)->getId();
                        seg.skin_data->vao_cache->attach(id, VFMT::ENUM_GENERIC::UVLightmap, 
                            VFMT::UVLightmap::count, VFMT::UVLightmap::gl_type, 
                            VFMT::UVLightmap::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                    }
                    if(seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::ColorRGBA)) {
                        GLuint id = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::ColorRGBA)->getId();
                        seg.skin_data->vao_cache->attach(id, VFMT::ENUM_GENERIC::ColorRGBA, 
                            VFMT::ColorRGBA::count, VFMT::ColorRGBA::gl_type, 
                            VFMT::ColorRGBA::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                    }
                    GLuint index_buf_id = seg.mesh->mesh.getIndexBuffer()->getId();
                    seg.skin_data->vao_cache->attachIndexBuffer(index_buf_id);
                }
            });

            if(bone_count && seg.mesh) {
                for(uint32_t j = 0; j < bone_count; ++j) {
                    entity_id entity = r.readAttribRef();
                    gfxm::mat4 m = r.read<gfxm::mat4>();
                    seg.skin_data->bone_nodes.emplace_back(ecsEntityHandle(getEntityHdl().getWorld(), entity));
                    seg.skin_data->bind_transforms.emplace_back(m);
                }
            }
        }
    }
};


#include <btBulletDynamicsCommon.h>
class ecsCollisionShape : public ecsAttrib<ecsCollisionShape> {
    enum SHAPE {
        SPHERE, BOX, CAPSULE, CONE, CYLINDER
    };
    SHAPE type = SPHERE;

public:
    std::shared_ptr<btCollisionShape> shape;
    uint64_t                          collision_mask = 0;
    uint64_t                          collision_group = 0;

    ecsCollisionShape() {
        shape.reset(new btSphereShape(.5f));    
    }

    void write(ecsWorldWriteCtx& out) override {
        out.write<uint8_t>(type);
        gfxm::vec3 buf;
        btVector3 btvec;
        switch(type) {
        case SPHERE:
            buf.x = ((btSphereShape*)shape.get())->getRadius();
            break;
        case BOX:
            btvec = ((btBoxShape*)shape.get())->getHalfExtentsWithMargin();
            buf = gfxm::vec3(btvec.getX(), btvec.getY(), btvec.getZ());
            break;
        case CAPSULE:
            buf.x = ((btCapsuleShape*)shape.get())->getRadius();
            buf.y = ((btCapsuleShape*)shape.get())->getHalfHeight() * 2.0f;
        break;
        case CONE:
            buf.x = ((btConeShape*)shape.get())->getRadius();
            buf.y = ((btConeShape*)shape.get())->getHeight();
        break;
        case CYLINDER:
            btvec = ((btCylinderShape*)shape.get())->getHalfExtentsWithMargin();
            buf = gfxm::vec3(btvec.getX(), btvec.getY(), btvec.getZ());
        break;
        default: assert(false);
        };
        out.write(buf);
        out.write(collision_mask);
        out.write(collision_group);
    }
    void read(ecsWorldReadCtx& in) override {
        type = (SHAPE)in.read<uint8_t>();
        gfxm::vec3 buf = in.read<gfxm::vec3>();
        collision_mask = in.read<uint64_t>();
        collision_group = in.read<uint64_t>();
        switch(type) {
        case SPHERE:    shape.reset(new btSphereShape(buf.x)); break;
        case BOX:       shape.reset(new btBoxShape(btVector3(buf.x, buf.y, buf.z)));   break;
        case CAPSULE:   shape.reset(new btCapsuleShape(buf.x, buf.y));; break;
        case CONE:      shape.reset(new btConeShape(buf.x, buf.y));; break;
        case CYLINDER:  shape.reset(new btCylinderShape(btVector3(buf.x, buf.y, buf.z))); break;
        default: assert(false);
        };
    }

    void onGui(ecsWorld* world, entity_id ent) override {
        const char* current_name = "UNKNOWN";
        switch(type) {
        case SPHERE:    current_name = "Sphere";  break;
        case BOX:       current_name = "Box";   break;
        case CAPSULE:   current_name = "Capsule"; break;
        case CONE:      current_name = "Cone"; break;
        case CYLINDER:  current_name = "Cylinder"; break;
        default: assert(false);
        };
        if(ImGui::BeginCombo("type###CollisionShapeType", current_name)) {
            if(ImGui::Selectable("Sphere")) {
                type = SPHERE;
                shape.reset(new btSphereShape(0.5f));
                world->signalAttribUpdate(ent, this->get_id());
            }
            if(ImGui::Selectable("Box")) {
                type = BOX;
                shape.reset(new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)));
                world->signalAttribUpdate(ent, this->get_id());
            }
            if(ImGui::Selectable("Capsule")) {
                type = CAPSULE;
                shape.reset(new btCapsuleShape(0.5f, 1.0f));
                world->signalAttribUpdate(ent, this->get_id());
            }
            if(ImGui::Selectable("Cone")) {
                type = CONE;
                shape.reset(new btConeShape(0.5f, 1.0f));
                world->signalAttribUpdate(ent, this->get_id());
            }
            if(ImGui::Selectable("Cylinder")) {
                type = CYLINDER;
                shape.reset(new btCylinderShape(btVector3(0.5f, 0.5f, 0.5f)));
                world->signalAttribUpdate(ent, this->get_id());
            }
            ImGui::EndCombo();
        }

        switch(type) {
        case SPHERE: {
            btSphereShape* s = (btSphereShape*)shape.get();
            float r = s->getRadius();
            if(ImGui::DragFloat("radius###CollisionShape", &r, 0.01f)) {
                *s = btSphereShape(r);
                world->signalAttribUpdate(ent, this->get_id());
            }
        }   break;
        case BOX: {
            btBoxShape* b = (btBoxShape*)shape.get();
            btVector3 vec = b->getHalfExtentsWithMargin();
            if(ImGui::DragFloat3("extents###CollisionShape", (float*)&vec, 0.01f)) {
                *b = btBoxShape(vec);
                world->signalAttribUpdate(ent, this->get_id());
            }
        }   break;
        case CAPSULE: {
            btCapsuleShape* c = (btCapsuleShape*)shape.get();
            float h = c->getHalfHeight() * 2.0f;
            float r = c->getRadius();
            if(ImGui::DragFloat("height###CollisionShape1", &h, 0.01f)) {
                *c = btCapsuleShape(r, h);
                world->signalAttribUpdate(ent, this->get_id());
            }
            if(ImGui::DragFloat("radius###CollisionShape2", &r, 0.01f)) {
                *c = btCapsuleShape(r, h);
                world->signalAttribUpdate(ent, this->get_id());
            }
        }    break;
        case CONE: {
            btConeShape* c = (btConeShape*)shape.get();
            float r = c->getRadius();
            float h = c->getHeight();
            if(ImGui::DragFloat("height###CollisionShape1", &h, 0.01f)) {
                *c = btConeShape(r, h);
                world->signalAttribUpdate(ent, this->get_id());
            }
            if(ImGui::DragFloat("radius###CollisionShape2", &r, 0.01f)) {
                *c = btConeShape(r, h);
                world->signalAttribUpdate(ent, this->get_id());
            }
        }   break;
        case CYLINDER: {
            btCylinderShape* c = (btCylinderShape*)shape.get();
            btVector3 vec = c->getHalfExtentsWithMargin();
            if(ImGui::DragFloat3("extents###CollisionShape", (float*)&vec, 0.01f)) {
                *c = btCylinderShape(vec);
                world->signalAttribUpdate(ent, this->get_id());
            }
        }   break;
        default:
            assert(false);
            break;
        };
    }
};
class ecsCollisionPlane : public ecsAttrib<ecsCollisionPlane> {
    gfxm::vec3 euler;

    gfxm::vec3 eulerToNormal(const gfxm::vec3& euler) {
        gfxm::vec3 N = gfxm::to_mat4(gfxm::euler_to_quat(euler)) * gfxm::vec4(0, 1, 0, 0);
        N = gfxm::normalize(N);
        return N;
    }
public:
    std::shared_ptr<btCollisionShape> shape;

    ecsCollisionPlane() {
        gfxm::vec3 N = eulerToNormal(euler);
        shape.reset(new btStaticPlaneShape(btVector3(N.x, N.y, N.z), 0));
    }

    void write(ecsWorldWriteCtx& out) override {
        btStaticPlaneShape* p = ((btStaticPlaneShape*)shape.get());
        float c = p->getPlaneConstant();
        out.write(euler);
        out.write(c);
    }
    void read(ecsWorldReadCtx& in) override {
        gfxm::vec3 euler = in.read<gfxm::vec3>();
        float offset = in.read<float>();
        
        gfxm::vec3 N = eulerToNormal(euler);
        shape.reset(new btStaticPlaneShape(btVector3(N.x, N.y, N.z), offset));
    }

    void onGui(ecsWorld* world, entity_id ent) override {
        btStaticPlaneShape* p = ((btStaticPlaneShape*)shape.get());
        float c = p->getPlaneConstant();
        if(ImGui::DragFloat3("euler###PlaneNormal", (float*)&euler, 0.01f)) {
            gfxm::vec3 N = eulerToNormal(euler);
            shape.reset(new btStaticPlaneShape(btVector3(N.x, N.y, N.z), c));
            world->signalAttribUpdate(ent, this->get_id());
        }
        if(ImGui::DragFloat("offset###PlaneConstant", &c, 0.01f)) {
            gfxm::vec3 N = eulerToNormal(euler);
            shape.reset(new btStaticPlaneShape(btVector3(N.x, N.y, N.z), c));
            world->signalAttribUpdate(ent, this->get_id());
        }
    }
};
#include "../../resource/collision_mesh.hpp"
class ecsSubDynamicsSys;
class ecsDynamicsSys;
class ecsTplCollisionMesh;
class ecsCollisionMesh : public ecsAttrib<ecsCollisionMesh> {
friend ecsSubDynamicsSys;
friend ecsDynamicsSys;
friend ecsTplCollisionMesh;
    std::shared_ptr<CollisionMesh> mesh;
    std::shared_ptr<btCollisionShape> shape;
    std::shared_ptr<btTriangleIndexVertexArray> ivarray;

    void rebuildShape() {
        if(!mesh || mesh->indexCount() == 0) {
            shape.reset(new btEmptyShape);
        } else {
            ivarray.reset(new btTriangleIndexVertexArray(
                mesh->indexCount() / 3,
                (int32_t*)mesh->getIndexDataPtr(),
                sizeof(uint32_t) * 3,
                mesh->vertexCount(),
                (btScalar*)mesh->getVertexDataPtr(),
                sizeof(float) * 3
            ));
            shape.reset(new btBvhTriangleMeshShape(
                ivarray.get(), true
            ));            
        }
        if(getEntityHdl().isValid()) {
            getEntityHdl().signalUpdate<ecsCollisionMesh>();
        }
    }

public:
    ecsCollisionMesh() {
        rebuildShape();
    }

    void setMesh(std::shared_ptr<CollisionMesh> m) {
        mesh = m;
        rebuildShape();
    }
    std::shared_ptr<CollisionMesh>& getMesh() {
        return mesh;
    }

    void write(ecsWorldWriteCtx& out) override {
        out.writeResource(mesh);
    }
    void read(ecsWorldReadCtx& in) override {
        mesh = in.readResource<CollisionMesh>();
        rebuildShape();
    }
    void onGui(ecsWorld* world, entity_id ent) override {
        imguiResourceTreeCombo("mesh###CollisionMesh", mesh, "colmesh", [this](){
            rebuildShape();
        });
        if(getEntityHdl().findAttrib<ecsMeshes>()) {
            if(ImGui::Button("Build from rendered mesh", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {
                std::vector<gfxm::vec3> vertices;
                std::vector<uint32_t>   indices;
                auto meshes = getEntityHdl().findAttrib<ecsMeshes>();
                for(int i = 0; i < meshes->segmentCount(); ++i) {
                    auto& seg = meshes->getSegment(i);
                    if(seg.mesh) {
                        auto buf = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::Position);
                        size_t sz = seg.mesh->mesh.getAttribDataSize(VFMT::ENUM_GENERIC::Position);
                        if(buf && sz) {
                            std::vector<gfxm::vec3> vbuf(sz * sizeof(gfxm::vec3));
                            buf->extractData(vbuf.data(), 0, sz);
                            vertices.insert(vertices.end(), vbuf.begin(), vbuf.end());
                        }

                        auto ibuf = seg.mesh->mesh.getIndexBuffer();
                        size_t icount = seg.mesh->mesh.getIndexCount();
                        if(ibuf && icount) {
                            std::vector<uint32_t> index_buf(icount);
                            ibuf->copyData(index_buf.data(), icount * sizeof(uint32_t));
                            indices.insert(indices.end(), index_buf.begin(), index_buf.end());
                        }
                    }
                }

                if(vertices.size() && indices.size()) {
                    mesh.reset(new CollisionMesh);
                    mesh->setVertices(vertices.data(), vertices.size());
                    mesh->setIndices(indices.data(), indices.size());
                    rebuildShape();
                }
            }
        }
        if(getEntityHdl().findAttrib<ecsSubScene>()) {
            if(ImGui::Button("Build from subscene", ImVec2(ImGui::GetWindowContentRegionWidth(), .0f))) {

            }
        }
    }
};
#include "generic/collision_groups.hpp"
class ecsCollisionFilter : public ecsAttrib<ecsCollisionFilter> {
public:
    uint64_t group = 1;
    uint64_t mask = 1;

    void write(ecsWorldWriteCtx& out) override {
        out.write(group);
        out.write(mask);
    }
    void read(ecsWorldReadCtx& in) override {
        in.read(group);
        in.read(mask);
    }

    void onGui(ecsWorld* world, entity_id ent) override {
        int grp_count = sizeof(s_collision_group_names) / sizeof(s_collision_group_names[0]);
        std::string groups_str;
        std::string mask_str;
        for(int i = 0; i < grp_count; ++i) {
            if(group & (1 << i)) {
                if(i > 0) {
                    groups_str += ", ";
                }
                groups_str += s_collision_group_names[i];
            }
        }
        for(int i = 0; i < grp_count; ++i) {
            if(mask & (1 << i)) {
                if(i > 0) {
                    mask_str += ", ";
                }
                mask_str += s_collision_group_names[i];
            }
        }

        if(ImGui::BeginCombo("groups", groups_str.c_str())) {
            for(int i = 0; i < grp_count; ++i) {
                std::string label;
                label += (group & (1 << i)) ? ICON_MDI_CHECKBOX_MARKED : ICON_MDI_CHECKBOX_BLANK;
                label = MKSTR(label << " " << s_collision_group_names[i]);
                if(ImGui::Selectable(label.c_str())) {
                    group ^= (1 << i);
                    world->signalAttribUpdate(ent, this->get_id());
                }
            }
            ImGui::EndCombo();
        }
        if(ImGui::BeginCombo("mask", mask_str.c_str())) {
            for(int i = 0; i < grp_count; ++i) {
                std::string label;
                label += (mask & (1 << i)) ? ICON_MDI_CHECKBOX_MARKED : ICON_MDI_CHECKBOX_BLANK;
                label = MKSTR(label << " " << s_collision_group_names[i]);
                if(ImGui::Selectable(label.c_str())) {
                    mask ^= (1 << i);
                    world->signalAttribUpdate(ent, this->get_id());
                }
            }
            ImGui::EndCombo();
        }
    }
};
class ecsCollisionCache : public ecsAttrib<ecsCollisionCache> {
public:
    struct CollisionPoint {
        gfxm::vec3 pointOnA; // A is the cache owner
        gfxm::vec3 pointOnB;
        gfxm::vec3 normalOnB;
    };
    struct Other {
        ecsEntityHandle entity;
        std::vector<CollisionPoint> points;
    };
    std::vector<Other> entities;
};
class ecsKinematicCharacter : public ecsAttrib<ecsKinematicCharacter> {
public:
    int val;
};
class ecsTestAttrib : public ecsAttrib<ecsTestAttrib> {
    int val;
public:
    ecsTestAttrib() {
        LOG_WARN("ecsTestAttrib()");
    }
    ecsTestAttrib(const ecsTestAttrib& other) {
        LOG_WARN("ecsTestAttrib(const ecsTestAttrib& other)");
    }
    ~ecsTestAttrib() {
        LOG_WARN("~ecsTestAttrib()");
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
class ecsLightDirect : public ecsAttrib<ecsLightDirect> {
public:
    gfxm::vec3 color = gfxm::vec3(1,1,1);
    float      intensity;

    void onGui(ecsWorld* world, entity_id ent) override {
        ImGui::ColorEdit3(MKSTR("color##" << this).c_str(), (float*)&color);
        ImGui::DragFloat(MKSTR("intensity##" << this).c_str(), &intensity, 0.01f);
    }

    void write(ecsWorldWriteCtx& w) override {
        w.write(color);
        w.write(intensity);
    }
    void read(ecsWorldReadCtx& r) override {
        color = r.read<gfxm::vec3>();
        intensity = r.read<float>();
    }
};


class ecsAudioListener : public ecsAttrib<ecsAudioListener> {
public:
    void write(ecsWorldWriteCtx& out) override {
        out.write<uint64_t>(0); // reserved
        out.write<uint64_t>(0); // reserved
        out.write<uint64_t>(0); // reserved
        out.write<uint64_t>(0); // reserved
    }
    void read(ecsWorldReadCtx& in) override {
        in.read<uint64_t>(); // reserved
        in.read<uint64_t>(); // reserved
        in.read<uint64_t>(); // reserved
        in.read<uint64_t>(); // reserved
    }

    void onGui(ecsWorld* world, entity_id e) override {

    }
};
#include "../../resource/audio_clip.hpp"
class ecsAudioSource : public ecsAttrib<ecsAudioSource> {
public:
    enum CMD {
        NONE,
        PLAY,
        PAUSE,
        STOP 
    };

private:
    std::shared_ptr<AudioClip> clip;
    bool autoplay = false;
    bool looping = false;
    float volume = 1.0f;
    uint8_t layer = 0; // Music, sfx, etc.

    CMD cmd = NONE;
    bool playing = false;

public:
    AudioClip* getClip() {
        return clip.get();
    }
    bool isAutoplay() const {
        return autoplay;
    }
    bool isLooping() const {
        return looping;
    }
    bool isPlaying() const {
        return playing;
    }

    void _setPlaying(bool val) {
        playing = val;
    }
    CMD _getCmd() const {
        return cmd;
    }
    void _clearCmd() {
        cmd = NONE;
    }

    void Play() {
        cmd = PLAY;
        getEntityHdl().signalUpdate<ecsAudioSource>();
    }
    void Pause() {
        cmd = PAUSE;
        getEntityHdl().signalUpdate<ecsAudioSource>();
    }
    void Stop() {
        cmd = STOP;
        getEntityHdl().signalUpdate<ecsAudioSource>();
    }

    void write(ecsWorldWriteCtx& out) override {
        out.writeResource(clip);
        out.write<uint8_t>(autoplay ? 1 : 0);
        out.write<uint8_t>(looping ? 1 : 0);
        out.write(volume);
        out.write(layer);
    }
    void read(ecsWorldReadCtx& in) override {
        clip = in.readResource<AudioClip>();
        autoplay = in.read<uint8_t>() ? true : false;
        looping = in.read<uint8_t>() ? true : false;
        in.read(volume);
        in.read(layer);
    }

    void onGui(ecsWorld* world, entity_id e) override {
        imguiResourceTreeCombo("clip###AudioSourceClip", clip, "ogg", [this, world, e](){
            world->signalAttribUpdate(e, this->get_id());
        });
        if(ImGui::Checkbox("autoplay###AudioSourceAutoplay", &autoplay)) {
            world->signalAttribUpdate(e, this->get_id());
        }
        if(ImGui::Checkbox("looping###AudioSourceLooping", &looping)) {
            world->signalAttribUpdate(e, this->get_id());
        }
    }
};


#endif
