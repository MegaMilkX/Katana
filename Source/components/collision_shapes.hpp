#ifndef COLLISION_SHAPES_HPP
#define COLLISION_SHAPES_HPP

#include <btBulletCollisionCommon.h>
#include "../gfxm.hpp"
#include "../util/serialization.hpp"

namespace Collision {

class Shape {
public:
    btCollisionShape* getBtShape() {
        return shape.get();
    }
protected:
    std::shared_ptr<btCollisionShape> shape;
};

class Box : public Shape {
public:
    Box() {
        shape.reset(new btBoxShape(btVector3(.5f, .5f, .5f)));
    }
    Box& operator=(const Box& other) {
        size = other.size;
        (*(btBoxShape*)shape.get()) = btBoxShape(btVector3(size.x, size.y, size.z));    
        return *this;
    }
    virtual void _editorGui() {
        if(ImGui::DragFloat3("Size##Box", (float*)&size, 0.01f)) {
            (*(btBoxShape*)shape.get()) = btBoxShape(btVector3(size.x, size.y, size.z));    
        }
    }

    virtual void serialize(std::ostream& out) {
        write(out, size);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        size = read<gfxm::vec3>(in);
        (*(btBoxShape*)shape.get()) = btBoxShape(btVector3(size.x, size.y, size.z));
    }
private:
    gfxm::vec3 size = gfxm::vec3(.5f, .5f, .5f);
};

class Sphere : public Shape {
public:
    Sphere() {
        shape.reset(new btSphereShape(.5f));
    }
    Sphere& operator=(const Sphere& other) {
        radius = other.radius;
        (*(btSphereShape*)shape.get()) = btSphereShape(radius);     
        return *this;
    }
    virtual void _editorGui() {
        if(ImGui::DragFloat("Radius##Sphere", &radius, 0.01f)) {
            (*(btSphereShape*)shape.get()) = btSphereShape(radius);    
        }
    }

    virtual void serialize(std::ostream& out) {
        write(out, radius);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        radius = read<float>(in);
        (*(btSphereShape*)shape.get()) = btSphereShape(radius); 
    }
private:
    float radius = .5f;
};

class Cylinder : public Shape {
public:
    Cylinder() {
        shape.reset(new btCylinderShape(btVector3(.5f, .5f, .5f)));
    }
    Cylinder& operator=(const Cylinder& other) {
        size = other.size;
        (*(btCylinderShape*)shape.get()) = btCylinderShape(btVector3(size.x, size.y, size.z));     
        return *this;
    }
    virtual void _editorGui() {
        if(ImGui::DragFloat3("Size##Cylinder", (float*)&size, 0.01f)) {
            (*(btCylinderShape*)shape.get()) = btCylinderShape(btVector3(size.x, size.y, size.z));    
        }
    }

    virtual void serialize(std::ostream& out) {
        write(out, size);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        size = read<gfxm::vec3>(in);
        (*(btCylinderShape*)shape.get()) = btCylinderShape(btVector3(size.x, size.y, size.z));    
    }
private:
    gfxm::vec3 size = gfxm::vec3(.5f, .5f, .5f);
};

class Capsule : public Shape {
public:
    Capsule() {
        shape.reset(new btCapsuleShape(.5f, .5f));
    }
    Capsule& operator=(const Capsule& other) {
        radius = other.radius;
        height = other.height;
        (*(btCapsuleShape*)shape.get()) = btCapsuleShape(radius, height); 
        return *this;
    }
    virtual void _editorGui() {
        if(ImGui::DragFloat("Radius##Capsule", &radius, 0.01f)) {
            (*(btCapsuleShape*)shape.get()) = btCapsuleShape(radius, height);   
        }
        if(ImGui::DragFloat("Height##Capsule", &height, 0.01f)) {
            (*(btCapsuleShape*)shape.get()) = btCapsuleShape(radius, height);   
        }
    }

    virtual void serialize(std::ostream& out) {
        write(out, radius);
        write(out, height);
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        radius = read<float>(in);
        height = read<float>(in);
        (*(btCapsuleShape*)shape.get()) = btCapsuleShape(radius, height);   
    }
private:
    float radius = .5f;
    float height = .5f;
};

}

#endif
