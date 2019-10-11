#ifndef WORLD_CONTROLLER_HPP
#define WORLD_CONTROLLER_HPP

class ktEcsSignature {
public:
    ktEcsSignature& inherit(const char* parent);
    template<typename T>
    ktEcsSignature& require();
    template<typename T>
    ktEcsSignature& optional();
    template<typename T>
    ktEcsSignature& exclude();
};

template<typename T>
void defineAttrib(const char* name);
ktEcsSignature& defineArchetype(const char* name);

struct Transform;
struct Parent;
struct Children;
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
        defineAttrib<Transform>("Transform");
        defineAttrib<Parent>("Parent");
        defineAttrib<Children>("Children");
        defineAttrib<Name>("Name");

        defineArchetype("Transform")
            .require<Transform>();
        defineArchetype("SceneGraphNode")
            .require<Transform>()
            .require<Parent>()
            .require<Children>();
        defineArchetype("SceneGraphRoot")
            .require<Transform>()
            .require<Children>();

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
