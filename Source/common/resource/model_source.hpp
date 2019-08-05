#ifndef MODEL_SOURCE_HPP
#define MODEL_SOURCE_HPP

#include "resource.h"

#include "mesh.hpp"
#include "mesh.hpp"
#include "texture2d.h"
#include "material.hpp"
#include "animation.hpp"
#include "skeleton.hpp"
#include "../scene/game_scene.hpp"

#include "../attributes/animation_stack.hpp"
#include "../attributes/model.hpp"

class aiScene;
class aiNode;
class ModelSource : public Resource {
    RTTR_ENABLE(Resource)

    void loadSkeleton(const aiScene* ai_scene);
    void loadMeshes(const aiScene* ai_scene);
    void loadAnims(const aiScene* ai_scene);
    void loadResources(const aiScene* ai_scene);
    void loadSceneGraph(const aiScene* ai_scene, aiNode* node, ktNode* o);
    void loadSceneGraph(const aiScene* ai_scene);
public:
    std::shared_ptr<GameScene> scene;
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Texture2D>> textures;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Animation>> anims;
    std::shared_ptr<Skeleton> skeleton;

    ModelSource();
    ~ModelSource();

    virtual bool deserialize(in_stream& in, size_t sz);

    bool unpack(const std::string& dir);
};
STATIC_RUN(ModelSource)
{
    rttr::registration::class_<ModelSource>("ModelSource")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

#endif
