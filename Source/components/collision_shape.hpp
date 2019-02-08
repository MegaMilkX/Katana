#ifndef COLLISION_SHAPE_HPP
#define COLLISION_SHAPE_HPP

#include <memory>
#include <rttr/type>
#include "../gfxm.hpp"
#include <btBulletCollisionCommon.h>

#include "../scene.hpp"
#include "../scene_object.hpp"

#include "../model.hpp"
#include "../transform.hpp"

#include "../util/serialization.hpp"

class Collider;

class CollisionShape {
public:
    enum TYPE {
        BOX,
        SPHERE,
        CAPSULE,
        CONE,
        MESH
    };

    virtual TYPE getShapeType() = 0;
    virtual btCollisionShape* getBtShape() = 0;
    virtual CollisionShape* clone() = 0;
    virtual rttr::type getType() = 0;
    virtual bool _editorGui(Collider*) = 0;

    virtual void serialize(std::ostream& out) {}
    virtual void deserialize(std::istream& in, size_t sz) {}

    virtual gfxm::vec3 getCenter() {
        return center;
    }
protected:
    gfxm::vec3 center;
};

class CollisionCapsule : public CollisionShape {
public:
    CollisionCapsule()
    : shape(0.5f, 1.6f) 
    {}

    virtual TYPE getShapeType() {
        return CAPSULE;
    }
    virtual btCollisionShape* getBtShape() {
        return &shape;
    }
    virtual CollisionShape* clone() {
        return new CollisionCapsule(*this);
    }
    virtual rttr::type getType() {
        return rttr::type::get<CollisionCapsule>();
    }
    virtual bool _editorGui(Collider*) {
        bool changed = false;
        if(ImGui::DragFloat3("Center", (float*)&center, 0.01f)) {
            changed = true;
        }
        if(ImGui::DragFloat("Radius", &radius, 0.01f)) {
            shape = btCapsuleShape(radius, height);    
        }
        if(ImGui::DragFloat("Height", &height, 0.01f)) {
            shape = btCapsuleShape(radius, height);
        }
        // TODO: For basic shapes
        if(ImGui::Button("Fit to model")) {

        }
        return changed;
    }

    virtual void serialize(std::ostream& out) {
        write(out, radius);
        write(out, height);
        write(out, center);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        radius = read<float>(in);
        height = read<float>(in);
        center = read<gfxm::vec3>(in);
        
        shape = btCapsuleShape(radius, height);    
    }
private:
    btCapsuleShape shape;
    float radius = 0.5f;
    float height = 1.6f;
};

class CollisionMesh : public CollisionShape {
public:
    CollisionMesh()
    {}

    void makeFromModel(Model* mdl) {
        if(mdl->mesh) {
            vertices.clear();
            indices.clear();

            auto msh = mdl->mesh;
            vertices.resize(msh->mesh.getAttribDataSize(gl::POSITION) / sizeof(gfxm::vec3));
            msh->mesh.copyAttribData(gl::POSITION, vertices.data());

            indices.resize(msh->mesh.getIndexDataSize() / sizeof(uint32_t));
            msh->mesh.copyIndexData(indices.data());

            gfxm::mat4 t = 
                gfxm::to_mat4(
                    gfxm::to_mat3(mdl->getObject()->get<Transform>()->getTransform())
                );
            for(auto& v : vertices) {
                gfxm::vec4 v4 = gfxm::vec4(v.x, v.y, v.z, 1.0f);
                v = t * v4;
            }

            makeMesh();
        }
    }

    virtual TYPE getShapeType() {
        return MESH;
    }
    virtual btCollisionShape* getBtShape() {
        if(mesh_shape) return mesh_shape.get();
        return &empty_shape;
    }
    virtual CollisionShape* clone() {
        auto ptr = new CollisionMesh();
        if(mesh_shape) {
            ptr->vertices = vertices;
            ptr->indices = indices;
            ptr->makeMesh();
        }
        return ptr;
    }
    virtual rttr::type getType() {
        return rttr::type::get<CollisionMesh>();
    }
    virtual bool _editorGui(Collider*);

    virtual void serialize(std::ostream& out) {
        write(out, center);
        write(out, (uint64_t)vertices.size());
        out.write((char*)vertices.data(), sizeof(gfxm::vec3) * vertices.size());
        write(out, (uint64_t)indices.size());
        out.write((char*)indices.data(), sizeof(uint32_t) * indices.size());
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        center = read<gfxm::vec3>(in);
        uint64_t v_count = read<uint64_t>(in);
        vertices.resize(v_count);
        in.read((char*)vertices.data(), v_count * sizeof(gfxm::vec3));
        uint64_t i_count = read<uint64_t>(in);
        indices.resize(i_count);
        in.read((char*)indices.data(), i_count * sizeof(uint32_t));

        makeMesh();
    }
private:
    void makeMesh() {
        indexVertexArray.reset(new btTriangleIndexVertexArray(
            indices.size() / 3,
            (int32_t*)indices.data(),
            sizeof(uint32_t) * 3,
            vertices.size() / sizeof(gfxm::vec3),
            (btScalar*)vertices.data(),
            sizeof(gfxm::vec3)
        ));
        mesh_shape.reset(new btBvhTriangleMeshShape(
            indexVertexArray.get(), true
        ));
    }

    btEmptyShape empty_shape;
    std::shared_ptr<btBvhTriangleMeshShape> mesh_shape;
    std::shared_ptr<btTriangleIndexVertexArray> indexVertexArray;
    std::vector<gfxm::vec3> vertices;
    std::vector<uint32_t> indices;
};

#endif
