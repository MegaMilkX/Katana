#ifndef MODEL_MESH_MODIFIER_HPP
#define MODEL_MESH_MODIFIER_HPP

enum MESH_MODIFIER_TYPE {
    MESH_MODIFIER_TEST,
    MESH_MODIFIER_SKIN,
    MESH_MODIFIER_WAVE,

    MESH_MODIFIER_COUNT
};

class ModelMeshModifier {
public:
    virtual ~ModelMeshModifier() {}

    virtual void run() = 0;
};

#endif
