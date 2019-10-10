#ifndef WORLD_CONTROLLER_HPP
#define WORLD_CONTROLLER_HPP

class ktEcsSignature {
public:
    template<typename T>
    ktEcsSignature& require();
    template<typename T>
    ktEcsSignature& optional();
    template<typename T>
    ktEcsSignature& exclude();
};

struct Transform;
struct Name;
struct Collider;
struct RigidBody;
struct Model;
struct Skin;

class WorldController {
public:
    virtual ~WorldController() {}
};

class wctrlTest : public WorldController {
public:
    wctrlTest() {
        ktEcsSignature sigCollider;
        sigCollider
            .require<Transform>()
            .require<Collider>()
            .exclude<RigidBody>();
        ktEcsSignature sigRigidBody;
        sigRigidBody
            .require<Transform>()
            .require<Collider>()
            .require<RigidBody>();
    }
};

#endif
