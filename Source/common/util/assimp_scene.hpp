#ifndef ASSIMP_SCENE_HPP
#define ASSIMP_SCENE_HPP

#include <memory>
#include <string>
#include <vector>

class aiScene;
class aiMesh;
class AssimpScene {
    const aiScene* ai_scene = 0;
    std::vector<aiMesh*> meshes_with_fallback_uv;
public:
    AssimpScene(const void* data, size_t sz, const std::string& fname_hint);
    AssimpScene(const std::string& filename);
    ~AssimpScene();

    const aiScene* getScene() const { return ai_scene; }
};

#endif
