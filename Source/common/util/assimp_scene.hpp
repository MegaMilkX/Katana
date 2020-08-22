#ifndef ASSIMP_SCENE_HPP
#define ASSIMP_SCENE_HPP

#include <memory>
#include <string>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

class aiScene;
class aiMesh;
class AssimpScene {
    const aiScene* ai_scene = 0;
    std::vector<aiMesh*> meshes_with_fallback_uv;
public:
    AssimpScene() {}
    AssimpScene(const void* data, size_t sz, const std::string& fname_hint);
    AssimpScene(const std::string& filename);
    ~AssimpScene();

    bool load(const void* data, size_t sz, const std::string& fname_hint);

    const aiScene* getScene() const { return ai_scene; }
};

#endif
