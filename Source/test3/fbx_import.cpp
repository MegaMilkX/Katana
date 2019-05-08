#include "fbx_import.hpp"

#include <fstream>

#include "../common/util/log.hpp"
#include "../common/gfxm.hpp"

static gfxm::vec2 sampleSphericalMap(gfxm::vec3 v) {
    const gfxm::vec2 invAtan = gfxm::vec2(0.1591f, 0.3183f);
    gfxm::vec2 uv = gfxm::vec2(atan2f(v.z, v.x), asinf(v.y));
    uv *= invAtan;
    uv = gfxm::vec2(uv.x + 0.5, uv.y + 0.5);
    return uv;
}

static void calcMissingUV(const aiScene* ai_scene) {
    std::vector<aiMesh*> meshes_with_fallback_uv;
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
}

static float getScaleFactor(const aiScene* ai_scene) {
    double scaleFactor = 1.0;
    if(ai_scene->mMetaData) {
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0;
            scaleFactor *= 0.01;
        }
    }
    return (float)scaleFactor;
}

bool importFbx(
    const std::vector<char>& buf,
    SceneObject* o,
    const std::string& fname
) {
    const aiScene* ai_scene = aiImportFileFromMemory(
        buf.data(), buf.size(),
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale,
        fname.c_str()
    );
    if(!ai_scene) {
        LOG_WARN("aiImportFileFromMemory failed: " << fname);
        return false;
    }

    calcMissingUV(ai_scene);

    ai_scene = aiApplyPostProcessing(
        ai_scene,
        aiProcess_CalcTangentSpace
    );
    if(!ai_scene) {
        LOG_WARN("aiApplyPostProcessing failed: " << fname);
        return false;
    }

    aiNode* ai_rootNode = ai_scene->mRootNode;
    if(!ai_rootNode) {
        LOG_WARN("Import: no root node: " << fname);
        return false;
    }
    float scaleFactor = getScaleFactor(ai_scene);

    // TODO: Create object

    aiReleaseImport(ai_scene);
    return true;
}

bool importFbx(const std::string& fname, SceneObject* o) {
    std::ifstream f(fname, std::ios::binary | std::ios::ate);
    if(!f.is_open()) {
        LOG_WARN("Failed to open " << fname);
        return false;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buf((unsigned int)size);
    if(!f.read(buf.data(), (unsigned int)size)) {
        f.close();
        return false;
    }
    return importFbx(buf, o, fname);
}