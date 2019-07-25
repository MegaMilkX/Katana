#include "assimp_scene.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "log.hpp"

static gfxm::vec2 sampleSphericalMap(gfxm::vec3 v) {
    const gfxm::vec2 invAtan = gfxm::vec2(0.1591f, 0.3183f);
    gfxm::vec2 uv = gfxm::vec2(atan2f(v.z, v.x), asinf(v.y));
    uv *= invAtan;
    uv = gfxm::vec2(uv.x + 0.5, uv.y + 0.5);
    return uv;
}

AssimpScene::AssimpScene(const void* data, size_t sz, const std::string& fname_hint) {
    ai_scene = aiImportFileFromMemory(
        (const char*)data, sz,
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale,
        fname_hint.c_str()
    );

    unsigned int mesh_count = ai_scene->mNumMeshes;
    for(unsigned int i = 0; i < mesh_count; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[i];

        if(ai_mesh->HasTextureCoords(0) == false) {
            meshes_with_fallback_uv.emplace_back(ai_mesh);
            ai_mesh->mTextureCoords[0] = new aiVector3D[ai_mesh->mNumVertices];
            // Generate uvs
            LOG("No uvs found, generating...")
            std::vector<float> uvs;
            for(unsigned j = 0; j < ai_mesh->mNumVertices; ++j) {
                gfxm::vec2 uv = sampleSphericalMap(
                    gfxm::normalize(gfxm::vec3(
                        ai_mesh->mVertices[j].x,
                        ai_mesh->mVertices[j].y,
                        ai_mesh->mVertices[j].z
                    ))
                );
                ai_mesh->mTextureCoords[0][j].x = uv.x;
                ai_mesh->mTextureCoords[0][j].y = uv.y;
            }
            LOG("Done");
        }
    }

    ai_scene = aiApplyPostProcessing(
        ai_scene,
        aiProcess_CalcTangentSpace
    );

    if(!ai_scene) {
        LOG_WARN("Failed to import " << fname_hint);
        return;
    }
}
AssimpScene::AssimpScene(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    if(!f.is_open()) {
        LOG_WARN("Failed to open model file " << filename);
        return;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer((unsigned int) size);
    if(!f.read(buffer.data(), (unsigned int)size)) {
        f.close();
        return;
    }

    *this = AssimpScene(buffer.data(), buffer.size(), filename);
}
AssimpScene::~AssimpScene() {
    for(auto m : meshes_with_fallback_uv) {
        delete[] m->mTextureCoords[0];
        m->mTextureCoords[0] = 0;
    }
    aiReleaseImport(ai_scene);
}