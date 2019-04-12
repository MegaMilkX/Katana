#include "collision_shape.hpp"

#include "../scene/game_object.hpp"
#include "../scene/game_scene.hpp"
#include "../scene/controllers/dynamics_ctrl.hpp"
#include "model.hpp"
#include "../../common/resource/mesh.hpp"

STATIC_RUN(CollisionShape) {
    rttr::registration::class_<CollisionShape>("CollisionShape")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

void MeshShape::onGui() {
    ImGui::Text("Debug display for collision meshes is disabled for performance");
    imguiResourceCombo<Mesh>(
        "Collision mesh",
        mesh,
        ".msh",
        [this](){
            setMesh(mesh);
        }
    );
    if(ImGui::Button("Make from model")) {
        makeFromModel();
    }
}

void MeshShape::makeFromModel() {
    makeFromModel(shape_wrapper->getOwner()->find<Model>());
}
void MeshShape::makeFromModel(std::shared_ptr<Model> mdl) {
    if(!mdl) return;
    if(mdl->segmentCount() == 0) return;
    auto& seg = mdl->getSegment(0);
    setMesh(seg.mesh);
    /*
    vertices.clear();
    indices.clear();
    for(size_t i = 0; i < mdl->segmentCount(); ++i) {
        auto& seg = mdl->getSegment(i);
        std::vector<gfxm::vec3> temp_vertices;
        std::vector<uint32_t> temp_indices;
        if(!seg.mesh) continue;

        auto msh = seg.mesh;
        temp_vertices.resize(msh->mesh.getAttribDataSize(gl::POSITION) / sizeof(gfxm::vec3));
        msh->mesh.copyAttribData(gl::POSITION, temp_vertices.data());
        temp_indices.resize(msh->mesh.getIndexDataSize() / sizeof(uint32_t));
        msh->mesh.copyIndexData(temp_indices.data());
        for(size_t j = 0; j < temp_indices.size(); ++j) {
            temp_indices[j] += vertices.size();
        }
        vertices.insert(vertices.end(), temp_vertices.begin(), temp_vertices.end());
        indices.insert(indices.end(), temp_indices.begin(), temp_indices.end());
    }
    if(!indices.empty() && !vertices.empty()) {
        makeMesh();
    }*/
}/*
void MeshShape::makeMesh() {
    indexVertexArray.reset(new btTriangleIndexVertexArray(
        indices.size() / 3,
        (int32_t*)indices.data(),
        sizeof(uint32_t) * 3,
        vertices.size() / sizeof(gfxm::vec3),
        (btScalar*)vertices.data(),
        sizeof(gfxm::vec3)
    ));
    bt_mesh_shape.reset(new btBvhTriangleMeshShape(
        indexVertexArray.get(), true
    ));
    shape = bt_mesh_shape.get();

    shape_wrapper->_shapeChanged();
}*/

void MeshShape::serialize(out_stream& out) {
    DataWriter w(&out);
    if(mesh) {
        w.write(mesh->Name());
    } else {
        w.write(std::string());
    }
}
void MeshShape::deserialize(in_stream& in) {
    DataReader r(&in);
    std::string m_name = r.readStr();
    if(!m_name.empty()) {
        setMesh(m_name);
    }
}

void MeshShape::setMesh(const std::string& res_name) {
    setMesh(retrieve<Mesh>(res_name));
}
void MeshShape::setMesh(std::shared_ptr<Mesh> mesh) {
    if(!mesh) return;
    if(!(mesh->vertexCount() && mesh->indexCount())) {
        LOG_WARN("Can't use mesh without position or indices as collision mesh");
        return;
    }
    this->mesh = mesh;
    indexVertexArray.reset(new btTriangleIndexVertexArray(
        mesh->indexCount() / 3,
        (int32_t*)mesh->getPermanentIndexData(),
        sizeof(uint32_t) * 3,
        mesh->vertexCount(),
        (btScalar*)mesh->getPermanentVertexData(),
        sizeof(float) * 3
    ));
    bt_mesh_shape.reset(new btBvhTriangleMeshShape(
        indexVertexArray.get(), true
    ));
    shape = bt_mesh_shape.get();

    shape_wrapper->_shapeChanged();
}

CollisionShape::~CollisionShape() {
}

void CollisionShape::onCreate() {
    getOwner()->getScene()->getController<DynamicsCtrl>();
}

void CollisionShape::debugDraw(btIDebugDraw* dd) {
    if(!shape) return;
    shape->debugDraw(dd, getOwner()->getTransform()->getWorldTransform());
}

void CollisionShape::_shapeChanged() {
    getOwner()->getScene()->getController<DynamicsCtrl>()->_shapeChanged(this);
}