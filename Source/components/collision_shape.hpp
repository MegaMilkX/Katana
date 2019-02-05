#ifndef COLLISION_SHAPE_HPP
#define COLLISION_SHAPE_HPP

class CollisionShape {
public:
    virtual void _editorGui() = 0;
};

class CollisionCapsule {
public:
    virtual void _editorGui() {

    }
};

class CollisionMesh {
public:
    virtual void _editorGui() {
        
    }
};

#endif
