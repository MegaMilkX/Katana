#ifndef SCENE_OBJECT_FROM_FBX_HPP
#define SCENE_OBJECT_FROM_FBX_HPP

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>

#include "log.hpp"
#include "split.hpp"
#include "filesystem.hpp"
#include "../asset_params.hpp"

//#include "../resource/data_registry.h"
#include "../resource/resource_tree.hpp"

#include "../resource/skeleton.hpp"
#include "../resource/animation.hpp"
#include "../resource/mesh.hpp"

#include "../attributes/model.hpp"
#include "../attributes/animation_stack.hpp"

#include "../scene/node.hpp"

#include "../platform/platform.hpp"

inline gfxm::vec2 sampleSphericalMap(gfxm::vec3 v) {
    const gfxm::vec2 invAtan = gfxm::vec2(0.1591f, 0.3183f);
    gfxm::vec2 uv = gfxm::vec2(atan2f(v.z, v.x), asinf(v.y));
    uv *= invAtan;
    uv = gfxm::vec2(uv.x + 0.5, uv.y + 0.5);
    return uv;
}

inline void regFilesystemResource(const std::string& fname) {
    gResourceTree.insert(fname, new DataSourceFilesystem(get_module_dir()  + "/" + platformGetConfig().data_dir + "/" +  fname));
}

inline std::shared_ptr<Skeleton> skeletonFromAssimpScene(const aiScene* ai_scene, const std::string& dirname, const std::string& root_name) {
    std::string fname = MKSTR("data/skel/" << dirname << "/" << root_name << ".skl");

    std::shared_ptr<Skeleton> skel = retrieve<Skeleton>(fname);
    if(!skel) {
        skel.reset(new Skeleton());
    }
    skel->clearBones();

    unsigned int mesh_count = ai_scene->mNumMeshes;
    for(unsigned int i = 0; i < mesh_count; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
            unsigned int bone_index = j;
            aiBone* bone = ai_mesh->mBones[j];
            skel->addBone(bone->mName.C_Str());
        }
    }
    for(unsigned i = 0; i < ai_scene->mNumAnimations; ++i) {
        aiAnimation* ai_anim = ai_scene->mAnimations[i];
        for(unsigned j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];
            skel->addBone(ai_node_anim->mNodeName.C_Str());
        }
    }
    std::function<void(aiNode*, std::shared_ptr<Skeleton>)> finalizeBone;
    finalizeBone = [&finalizeBone](aiNode* node, std::shared_ptr<Skeleton> skel) {
        for(unsigned i = 0; i < node->mNumChildren; ++i) {
            //skel->addBone(node->mChildren[i]->mName.C_Str());
            finalizeBone(node->mChildren[i], skel);
            gfxm::mat4 t = gfxm::transpose(*(gfxm::mat4*)&node->mChildren[i]->mTransformation);
            //LOG(node->mChildren[i]->mName.C_Str());
            //LOG(t);
            skel->setDefaultPose(
                node->mChildren[i]->mName.C_Str(), 
                t
            );
            skel->setParent(node->mChildren[i]->mName.C_Str(), node->mName.C_Str());
        }
    };
    finalizeBone(ai_scene->mRootNode, skel);

    skel->write_to_file(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + fname);
    regFilesystemResource(fname);

    return skel;
}

inline void meshesFromAssimpScene(
    const aiScene* ai_scene,
    AssetParams& asset_params,
    const std::string& dirname,
    const std::string& root_name
) {
    unsigned int mesh_count = ai_scene->mNumMeshes;
    for(unsigned int i = 0; i < mesh_count; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[i];
        int32_t vertexCount = ai_mesh->mNumVertices;
        int32_t triCount = ai_mesh->mNumFaces;
        int32_t indexCount = triCount * 3;
        std::vector<float> vertices;
        std::vector<uint32_t> indices;
        std::vector<std::vector<float>> normal_layers;
        normal_layers.resize(1);
        std::vector<float> tangent;
        std::vector<float> bitangent;
        std::vector<std::vector<float>> uv_layers;
        std::vector<std::vector<float>> rgb_layers;
        std::vector<gfxm::vec4> boneIndices;
        std::vector<gfxm::vec4> boneWeights;
        for(unsigned j = 0; j < vertexCount; ++j) {
            aiVector3D& v = ai_mesh->mVertices[j];
            vertices.emplace_back(v.x);
            vertices.emplace_back(v.y);
            vertices.emplace_back(v.z);
        }
        for(unsigned j = 0; j < triCount; ++j) {
            aiFace& f = ai_mesh->mFaces[j];
            for(unsigned k = 0; k < f.mNumIndices; ++k) {
                indices.emplace_back(f.mIndices[k]);
            }
        }
        for(unsigned j = 0; j < vertexCount; ++j) {
            aiVector3D& n = ai_mesh->mNormals[j];
            normal_layers[0].emplace_back(n.x);
            normal_layers[0].emplace_back(n.y);
            normal_layers[0].emplace_back(n.z);
        }
        for(unsigned j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j) {
            if(!ai_mesh->HasTextureCoords(j)) break;
            std::vector<float> uv_layer;
            for(unsigned k = 0; k < vertexCount; ++k) {
                aiVector3D& uv = ai_mesh->mTextureCoords[j][k];
                uv_layer.emplace_back(uv.x);
                uv_layer.emplace_back(uv.y);
            }
            uv_layers.emplace_back(uv_layer);
        }

        if(ai_mesh->mNumBones) {
            boneIndices.resize(vertexCount);
            boneWeights.resize(vertexCount);
            for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                unsigned int bone_index = j;
                aiBone* bone = ai_mesh->mBones[j];
                
                for(unsigned k = 0; k < bone->mNumWeights; ++k) {
                    aiVertexWeight& w = bone->mWeights[k];
                    gfxm::vec4& indices_ref = boneIndices[w.mVertexId];
                    gfxm::vec4& weights_ref = boneWeights[w.mVertexId];
                    for(unsigned l = 0; l < 4; ++l) {
                        if(weights_ref[l] == 0.0f) {
                            indices_ref[l] = (float)bone_index;
                            weights_ref[l] = w.mWeight;
                            break;
                        }
                    }
                }
            }
        }

        tangent.resize(vertexCount * 3);
        bitangent.resize(vertexCount * 3);
        if(ai_mesh->mTangents && ai_mesh->mBitangents) {
            for(unsigned j = 0; j < vertexCount; ++j) {
                aiVector3D& n = ai_mesh->mTangents[j];
                tangent[j * 3] = (n.x);
                tangent[j * 3 + 1] = (n.y);
                tangent[j * 3 + 2] = (n.z);
            }
            for(unsigned j = 0; j < vertexCount; ++j) {
                aiVector3D& n = ai_mesh->mBitangents[j];
                bitangent[j * 3] = (n.x);
                bitangent[j * 3 + 1] = (n.y);
                bitangent[j * 3 + 2] = (n.z);
            }
        }        

        // TODO: Preload before building to retain extra data
        std::shared_ptr<Mesh> mesh_ref(new Mesh());
        mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(float));
        if(normal_layers.size() > 0) {
            mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::Normal, normal_layers[0].data(), normal_layers[0].size() * sizeof(float));
        }
        if(uv_layers.size() > 0) {
            mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float));
        }
        if(!boneIndices.empty() && !boneWeights.empty()) {
            mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::BoneIndex4, boneIndices.data(), boneIndices.size() * sizeof(gfxm::vec4));
            mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::BoneWeight4, boneWeights.data(), boneWeights.size() * sizeof(gfxm::vec4));
        }
        mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::Tangent, tangent.data(), tangent.size() * sizeof(float));
        mesh_ref->mesh.setAttribData(VFMT::ENUM_GENERIC::Bitangent, bitangent.data(), bitangent.size() * sizeof(float));

        mesh_ref->mesh.setIndices(indices.data(), indices.size());

        std::string fname = MKSTR("data/mesh/" << dirname << "/" << root_name << i << ".msh");
        mesh_ref->write_to_file(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + fname);
        regFilesystemResource(fname);
    }
}

inline void extractRootMotionFromAssimpAnimNode(
    aiNodeAnim* ai_node_anim, 
    AnimNode& node, 
    AnimNode& rm_node, 
    std::shared_ptr<Skeleton> skeleton,
    bool root_translation,
    bool root_rotation
) {
    std::string node_name = ai_node_anim->mNodeName.C_Str();
    Skeleton::Bone* bone = skeleton->getBone(node_name);
    LOG("extractRootMotionFromAssimpAnimNode bone: " << node_name);

    // Get transforms
    gfxm::mat4 parent_transform = skeleton->getParentTransform(bone->name);
    gfxm::mat4 bone_w_transform = skeleton->getWorldTransform(bone->name);
    gfxm::mat4 bone_transform = bone->bind_pose;//skeleton->getWorldTransform(bone->name);
    gfxm::mat3 m3 = gfxm::to_mat3(bone_transform);
    m3[0] /= gfxm::length(m3[0]);
    m3[1] /= gfxm::length(m3[1]);
    m3[2] /= gfxm::length(m3[2]);
    gfxm::quat bone_rot = gfxm::to_quat(m3);
    m3 = gfxm::to_mat3(bone_w_transform);
    m3[0] /= gfxm::length(m3[0]);
    m3[1] /= gfxm::length(m3[1]);
    m3[2] /= gfxm::length(m3[2]);
    gfxm::quat bone_rot_w = gfxm::to_quat(m3);

    // Get rotation curve as is
    curve<gfxm::quat> r_curve;
    for(unsigned k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
        auto& ai_v = ai_node_anim->mRotationKeys[k].mValue;
        float t = (float)ai_node_anim->mRotationKeys[k].mTime;
        gfxm::quat o_q = { ai_v.x, ai_v.y, ai_v.z, ai_v.w };
        r_curve[t] = o_q;
    }

    if(!root_translation) {
        rm_node.t[0.0f] = gfxm::vec3(.0f, .0f, .0f);
        rm_node.t[1.0f] = gfxm::vec3(.0f, .0f, .0f);
    } else {
        auto& ai_v = ai_node_anim->mPositionKeys[0].mValue;
        gfxm::vec4 original_v = { ai_v.x, ai_v.y, ai_v.z, 1.0f };
        node.t[0.0f] = original_v;
        node.t[1.0f] = original_v;
    }
    if(!root_rotation) {
        rm_node.r[0.0f] = gfxm::quat(.0f, .0f, .0f, 1.0f);
        rm_node.r[1.0f] = gfxm::quat(.0f, .0f, .0f, 1.0f);
    } else {
        node.r[0.0f] = bone_rot;
        node.r[1.0f] = bone_rot;
        rm_node.r[0.0f] = bone_rot;
    }
    rm_node.s[0.0f] = gfxm::vec3(1.0f, 1.0f, 1.0f);
    rm_node.s[1.0f] = gfxm::vec3(1.0f, 1.0f, 1.0f);
    
    for(unsigned k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
        auto& ai_v = ai_node_anim->mPositionKeys[k].mValue;
        float t = (float)ai_node_anim->mPositionKeys[k].mTime;
        gfxm::vec4 original_v = { ai_v.x, ai_v.y, ai_v.z, 0.0f };

        if(root_translation) {
            gfxm::quat anim_rot = r_curve.at(t);
            gfxm::mat4 anim_rot_m4 = gfxm::to_mat4(anim_rot);

            gfxm::vec4 transformed_v = 
                parent_transform * original_v;

            gfxm::vec4 rm_v = 
                gfxm::vec4(transformed_v.x, .0f, transformed_v.z, 1.0f);
            gfxm::vec4 v = 
                gfxm::inverse(parent_transform) * gfxm::vec4(.0f, transformed_v.y, .0f, 1.0f);
            
            node.t[t] = v;
            rm_node.t[t] = rm_v;
        } else {
            node.t[t] = original_v;
        }
    }
    
    for(unsigned k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
        auto& ai_v = ai_node_anim->mRotationKeys[k].mValue;
        float t = (float)ai_node_anim->mRotationKeys[k].mTime;
        gfxm::quat o_q = { ai_v.x, ai_v.y, ai_v.z, ai_v.w };

        if(root_rotation) {
            rm_node.r[t] = gfxm::inverse(bone_rot) * o_q * bone_rot_w;
        } else {
            node.r[t] = o_q;
        }
    }
    for(unsigned k = 0; k < ai_node_anim->mNumScalingKeys; ++k) {
        auto& ai_v = ai_node_anim->mScalingKeys[k].mValue;
        float t = (float)ai_node_anim->mScalingKeys[k].mTime;
        gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
        node.s[t] = v;
    }
}

inline void animNodeFromAssimpNode(aiNodeAnim* ai_node_anim, AnimNode& node) {
    for(unsigned k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
        auto& ai_v = ai_node_anim->mPositionKeys[k].mValue;
        float t = (float)ai_node_anim->mPositionKeys[k].mTime;
        gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
        node.t[t] = v;
    }
    for(unsigned k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
        auto& ai_v = ai_node_anim->mRotationKeys[k].mValue;
        float t = (float)ai_node_anim->mRotationKeys[k].mTime;
        gfxm::quat v = { ai_v.x, ai_v.y, ai_v.z, ai_v.w };
        node.r[t] = v;
    }
    for(unsigned k = 0; k < ai_node_anim->mNumScalingKeys; ++k) {
        auto& ai_v = ai_node_anim->mScalingKeys[k].mValue;
        float t = (float)ai_node_anim->mScalingKeys[k].mTime;
        gfxm::vec3 v = { ai_v.x, ai_v.y, ai_v.z };
        node.s[t] = v;
    }
}

inline void animFromAssimpScene(
    const aiScene* ai_scene,
    std::shared_ptr<Skeleton> skeleton,
    AssetParams& asset_params,
    const std::string& dirname,
    const std::string& root_name
) {
    for(unsigned i = 0; i < ai_scene->mNumAnimations; ++i) {
        aiAnimation* ai_anim = ai_scene->mAnimations[i];
        double fps = ai_anim->mTicksPerSecond;
        double len = ai_anim->mDuration;

        AssetParams& anim_asset_params = 
            asset_params.get_object("Animation").get_object(ai_anim->mName.C_Str());
        bool is_additive = anim_asset_params.get_bool("Additive", false);
        bool enable_root_motion = anim_asset_params.get_bool("EnableRootMotion", false);
        std::string root_motion_node_name = anim_asset_params.get_string("RootMotionNode", "");
        bool root_rotation = anim_asset_params.get_bool("RootRotation", false);
        bool root_translation = anim_asset_params.get_bool("RootTranslation", false);

        std::string fname = MKSTR("data/anim/" << dirname << "/" << root_name << "_" << replace_reserved_chars(ai_anim->mName.C_Str(), '_') << ".anm");
        std::shared_ptr<Animation> anim = retrieve<Animation>(fname);
        if(!anim) {
            anim.reset(new Animation());
        }
        anim->clearNodes();
        anim->Storage(Resource::GLOBAL);
        anim->Name(ai_anim->mName.C_Str());

        anim->length                = (float)len;
        anim->fps                   = (float)fps;
        anim->root_motion_enabled   = enable_root_motion;
        anim->root_motion_node_name = root_motion_node_name;
        
        for(unsigned j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];
            std::string bone_node = ai_node_anim->mNodeName.C_Str();
            Skeleton::Bone* bone = skeleton->getBone(bone_node);
            if(!bone) continue;

            AnimNode& node = anim->getNode(bone_node);
            if(root_motion_node_name == bone_node) {
                extractRootMotionFromAssimpAnimNode(
                    ai_node_anim, 
                    node,       
                    anim->getRootMotionNode(),
                    skeleton,
                    root_translation,
                    root_rotation
                );
                continue;
            }
            animNodeFromAssimpNode(ai_node_anim, node);
        }

        if(is_additive) {
            // TODO: Convert to additive?
        }

        anim->write_to_file(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + fname);
        regFilesystemResource(fname);
    }
}

inline void resourcesFromAssimpScene(
    const aiScene* ai_scene,
    AssetParams& asset_params,
    const std::string& dirname,
    const std::string& root_name
) {
    auto skel = skeletonFromAssimpScene(ai_scene, dirname, root_name);
    meshesFromAssimpScene(ai_scene, asset_params, dirname, root_name);
    animFromAssimpScene(ai_scene, skel, asset_params, dirname, root_name);
}

inline void createGraphFromAssimpNode(aiNode* node, ktNode* o) {
    o->setName(node->mName.C_Str());
    o->getTransform()->setTransform(
        gfxm::transpose(*(gfxm::mat4*)&node->mTransformation)
    );
    
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        auto c = o->createChild();
        createGraphFromAssimpNode(node->mChildren[i], c);
    }
}

inline void finalizeObjectsFromAssimpNode(
    const aiScene* ai_scene,
    aiNode* node,
    ktNode* object,
    const std::string& dirname,
    const std::string& root_name
) {
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        auto co = object->getChild(node->mChildren[i]->mName.C_Str());
        finalizeObjectsFromAssimpNode(
            ai_scene,
            node->mChildren[i], 
            co, 
            dirname, 
            root_name
        );
    }

    ktNode* root_object = object->getRoot();

    if(node->mNumMeshes) {
        auto m = object->get<Model>();
        if(m) {
            for(unsigned i = 0; i < node->mNumMeshes; ++i) {
                auto& seg = m->getSegment(i);
                seg.mesh = retrieve<Mesh>(MKSTR("data/mesh/" << dirname << "/" << root_name << node->mMeshes[i] << ".msh"));
            
                aiMesh* ai_mesh = ai_scene->mMeshes[node->mMeshes[i]];
                if(ai_mesh->mNumBones) {
                    seg.skin_data.reset(new Model::SkinData());
                    for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                        aiBone* ai_bone = ai_mesh->mBones[j];
                        std::string name(ai_bone->mName.data, ai_bone->mName.length);

                        ktNode* o = root_object->findObject(name);
                        if(o) {
                            seg.skin_data->bone_nodes.emplace_back(o);
                            seg.skin_data->bind_transforms.emplace_back(
                                gfxm::transpose(*(gfxm::mat4*)&ai_bone->mOffsetMatrix)
                            );
                        }
                    }
                }
            }
        }
    }
}

inline bool objectFromFbx(const std::vector<char>& buffer, ktNode* o, const std::string& filename_hint = "") {
    auto sanitizeString = [](const std::string& str)->std::string {
        std::string name = str;
        for(size_t i = 0; i < name.size(); ++i) {
            name[i] = (std::tolower(name[i]));
            if(name[i] == '\\') {
                name[i] = '/';
            }
        }
        return name;
    };
    
    std::string filename = sanitizeString(filename_hint);

    std::string fname = filename;
    while(fname[fname.size() - 1] == '/') {
        fname = std::string(fname.begin(), fname.begin() + fname.size() - 1);
    }
    //std::replace( dirname.begin(), dirname.end(), '.', '_');
    fname = fname.substr(fname.find_last_of("/") + 1);
    fname = fname.substr(0, fname.find_first_of('.')); 
    //CreateDirectoryA(dirname.c_str(), 0);

    createDirRecursive(get_module_dir() + "/" + platformGetConfig().data_dir + "/data/anim/" + fname);
    createDirRecursive(get_module_dir() + "/" + platformGetConfig().data_dir + "/data/mesh/" + fname);
    createDirRecursive(get_module_dir() + "/" + platformGetConfig().data_dir + "/data/skel/" + fname);
    std::string asset_param_path = get_module_dir() + "/" + platformGetConfig().data_dir + "/asset_params/" + filename + ".asset_params";
    createDirRecursive(cut_dirpath(asset_param_path));

    std::string root_name = "object";
    std::vector<std::string> tokens = split(filename, '/');
    if(!tokens.empty()) {
        std::string name = tokens[tokens.size() - 1];
        tokens = split(name, '.');
        root_name = name.substr(0, name.find_last_of("."));
    }

    AssetParams asset_params;
    asset_params.load(asset_param_path);

    const aiScene* ai_scene = aiImportFileFromMemory(
        buffer.data(), buffer.size(),
        aiProcess_GenSmoothNormals |
        aiProcess_GenUVCoords |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale,
        filename.c_str()
    );

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

    ai_scene = aiApplyPostProcessing(
        ai_scene,
        aiProcess_CalcTangentSpace
    );

    if(!ai_scene) {
        LOG_WARN("Failed to import " << filename);
        return false;
    }
    aiNode* ai_rootNode = ai_scene->mRootNode;
    if(!ai_rootNode) {
        LOG_WARN("Fbx import: No root node (" << filename << ")");
        return false;
    }

    double scaleFactor = 1.0;
    if(ai_scene->mMetaData) {
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0f;
            scaleFactor *= 0.01;
            /*
            if(o->getParent()) {
                gfxm::vec3 a = root->get<Transform>()->scale();
                gfxm::vec3 b = root->getParent()->get<Transform>()->scale();
                root->get<Transform>()->scale(gfxm::vec3(
                    a.x / b.x, a.y / b.y, a.z / b.z
                ));
            }*/
        }
    }

    createGraphFromAssimpNode(ai_rootNode, o);
    o->setName(root_name);
    o->getTransform()->setScale((float)scaleFactor);
    resourcesFromAssimpScene(ai_scene, asset_params, fname, root_name);
    finalizeObjectsFromAssimpNode(ai_scene, ai_rootNode, o, fname, root_name);

    if(ai_scene->mNumAnimations > 0) {
        auto anim_stack = o->get<AnimationStack>();
        anim_stack->setSkeleton(retrieve<Skeleton>(
            MKSTR("data/skel/" << fname << "/" << root_name << ".skl")
        ));
    }
    for(unsigned i = 0; i < ai_scene->mNumAnimations; ++i) {
        auto anim_stack = o->get<AnimationStack>();
        std::string name = MKSTR("data/anim/" << fname << "/" << root_name << "_" << replace_reserved_chars(ai_scene->mAnimations[i]->mName.C_Str(), '_') << ".anm");
        anim_stack->addAnim(retrieve<Animation>(name));
    }

    o->refreshAabb();

    // Cleanup
    for(auto m : meshes_with_fallback_uv) {
        delete[] m->mTextureCoords[0];
        m->mTextureCoords[0] = 0;
    }
    aiReleaseImport(ai_scene);

    asset_params.write(asset_param_path);
    return true;
}

inline bool objectFromFbx(const std::string& filename, ktNode* o) {
    std::ifstream f(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + filename, std::ios::binary | std::ios::ate);
    if(!f.is_open()) {
        LOG_WARN("Failed to open " << filename);
        return false;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer((unsigned int)size);
    if(!f.read(buffer.data(), (unsigned int)size)) {
        f.close();
        return false;
    }
    return objectFromFbx(buffer, o, filename);
}

#endif
