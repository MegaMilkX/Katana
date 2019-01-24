#ifndef SCENE_FROM_FBX_HPP
#define SCENE_FROM_FBX_HPP

#include <functional>
#include <vector>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "scene.hpp"
#include "transform.hpp"
#include "model.hpp"
#include "skin.hpp"
#include "animator.hpp"
#include "util/log.hpp"
#include "util/split.hpp"
#include "asset_params.hpp"

inline void meshesFromAssimpScene(
    const aiScene* ai_scene, 
    Scene* scene, 
    std::shared_ptr<Skeleton> skeleton,
    AssetParams& asset_params
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
                skeleton->addBone(bone->mName.C_Str(), gfxm::mat4(1.0f));
                
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

        std::shared_ptr<Mesh> mesh_ref(new Mesh());
        mesh_ref->Name(MKSTR(i << ".geo"));
        mesh_ref->Storage(Resource::LOCAL);

        mesh_ref->mesh.setAttribData(gl::POSITION, vertices.data(), vertices.size() * sizeof(float), 3, GL_FLOAT, GL_FALSE);
        if(normal_layers.size() > 0) {
            mesh_ref->mesh.setAttribData(gl::NORMAL, normal_layers[0].data(), normal_layers[0].size() * sizeof(float), 3, GL_FLOAT, GL_FALSE);
        }
        if(uv_layers.size() > 0) {
            mesh_ref->mesh.setAttribData(gl::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float), 2, GL_FLOAT, GL_FALSE);
        }
        if(!boneIndices.empty() && !boneWeights.empty()) {
            mesh_ref->mesh.setAttribData(gl::BONE_INDEX4, boneIndices.data(), boneIndices.size() * sizeof(gfxm::vec4), 4, GL_FLOAT, GL_FALSE);
            mesh_ref->mesh.setAttribData(gl::BONE_WEIGHT4, boneWeights.data(), boneWeights.size() * sizeof(gfxm::vec4), 4, GL_FLOAT, GL_FALSE);
        }
        mesh_ref->mesh.setIndices(indices.data(), indices.size());

        scene->addLocalResource(mesh_ref);
    }
}

inline void animationsFromAssimpScene(
    const aiScene* ai_scene, 
    Scene* scene, 
    std::shared_ptr<Skeleton> skeleton,
    AssetParams& asset_params
) {
    for(unsigned int i = 0; i < ai_scene->mNumAnimations; ++i) {
        aiAnimation* ai_anim = ai_scene->mAnimations[i];
        double fps = ai_anim->mTicksPerSecond;
        double len = ai_anim->mDuration;
        std::shared_ptr<Animation> anim = std::make_shared<Animation>();
        anim->Storage(Resource::LOCAL);
        anim->Name(ai_anim->mName.C_Str());

        AssetParams anim_asset_params = asset_params.get_object("Animation").get_object(ai_anim->mName.C_Str());
        bool is_additive = anim_asset_params.get_bool("Additive", false);
        bool enable_root_motion = anim_asset_params.get_bool("EnableRootMotion", false);
        std::string root_motion_node_name = anim_asset_params.get_string("RootMotionNode", "");

        anim->root_motion_enabled = enable_root_motion;

        for(unsigned int j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];
            skeleton->addBone(ai_node_anim->mNodeName.C_Str(), gfxm::mat4(1.0f));
        }

        ozz::animation::offline::RawAnimation raw_anim;
        raw_anim.duration = (float)len;
        raw_anim.tracks.resize(skeleton->boneCount());

        for(unsigned int j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];
            std::string bone_node = ai_node_anim->mNodeName.C_Str();
            Skeleton::Bone* bone = skeleton->getBone(bone_node);
            if(!bone) continue;

            auto& track = raw_anim.tracks[bone->id];

            if(root_motion_node_name == ai_node_anim->mNodeName.C_Str()) {
                if(ai_node_anim->mNumPositionKeys) {
                    for(unsigned k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
                        float t = (float)ai_node_anim->mPositionKeys[k].mTime;
                        gfxm::vec3 v = {
                            ai_node_anim->mPositionKeys[k].mValue.x,
                            ai_node_anim->mPositionKeys[k].mValue.y,
                            ai_node_anim->mPositionKeys[k].mValue.z
                        };
                        anim->root_motion_pos[t] = v;
                    }
                    float t = (float)ai_node_anim->mPositionKeys[0].mTime;
                    ozz::math::Float3 v = {
                        ai_node_anim->mPositionKeys[0].mValue.x,
                        ai_node_anim->mPositionKeys[0].mValue.y,
                        ai_node_anim->mPositionKeys[0].mValue.z
                    };
                    ozz::animation::offline::RawAnimation::TranslationKey tkey = { t, v };
                    track.translations.push_back(tkey);
                }
                if(ai_node_anim->mNumRotationKeys) {
                    for(unsigned k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
                        float t = (float)ai_node_anim->mRotationKeys[k].mTime;
                        gfxm::quat v = {
                            ai_node_anim->mRotationKeys[k].mValue.x,
                            ai_node_anim->mRotationKeys[k].mValue.y,
                            ai_node_anim->mRotationKeys[k].mValue.z,
                            ai_node_anim->mRotationKeys[k].mValue.w
                        };
                        anim->root_motion_rot[t] = v;
                    }
                    float t = (float)ai_node_anim->mRotationKeys[0].mTime;
                    ozz::math::Quaternion v = {
                        ai_node_anim->mRotationKeys[0].mValue.x,
                        ai_node_anim->mRotationKeys[0].mValue.y,
                        ai_node_anim->mRotationKeys[0].mValue.z,
                        ai_node_anim->mRotationKeys[0].mValue.w
                    };
                    ozz::animation::offline::RawAnimation::RotationKey rkey = { t, v };
                    track.rotations.push_back(rkey);
                }
                if(ai_node_anim->mNumScalingKeys) {
                    float t = (float)ai_node_anim->mScalingKeys[0].mTime;
                    ozz::math::Float3 v = {
                        ai_node_anim->mScalingKeys[0].mValue.x,
                        ai_node_anim->mScalingKeys[0].mValue.y,
                        ai_node_anim->mScalingKeys[0].mValue.z
                    };
                    ozz::animation::offline::RawAnimation::ScaleKey skey = { t, v };
                    track.scales.push_back(skey);
                }    
                continue;
            }

            track.translations.reserve(ai_node_anim->mNumPositionKeys);
            for(unsigned k = 0; k < ai_node_anim->mNumPositionKeys; ++k) {
                float t = (float)ai_node_anim->mPositionKeys[k].mTime;
                ozz::math::Float3 v = {
                    ai_node_anim->mPositionKeys[k].mValue.x,
                    ai_node_anim->mPositionKeys[k].mValue.y,
                    ai_node_anim->mPositionKeys[k].mValue.z
                };
                ozz::animation::offline::RawAnimation::TranslationKey tkey = { t, v };
                track.translations.push_back(tkey);
            }
            track.rotations.reserve(ai_node_anim->mNumRotationKeys);
            for(unsigned k = 0; k < ai_node_anim->mNumRotationKeys; ++k) {
                float t = (float)ai_node_anim->mRotationKeys[k].mTime;
                ozz::math::Quaternion v = {
                    ai_node_anim->mRotationKeys[k].mValue.x,
                    ai_node_anim->mRotationKeys[k].mValue.y,
                    ai_node_anim->mRotationKeys[k].mValue.z,
                    ai_node_anim->mRotationKeys[k].mValue.w
                };
                ozz::animation::offline::RawAnimation::RotationKey rkey = { t, v };
                track.rotations.push_back(rkey);
            }
            track.scales.reserve(ai_node_anim->mNumScalingKeys);
            for(unsigned k = 0; k < ai_node_anim->mNumScalingKeys; ++k) {
                float t = (float)ai_node_anim->mScalingKeys[k].mTime;
                ozz::math::Float3 v = {
                    ai_node_anim->mScalingKeys[k].mValue.x,
                    ai_node_anim->mScalingKeys[k].mValue.y,
                    ai_node_anim->mScalingKeys[k].mValue.z
                };
                ozz::animation::offline::RawAnimation::ScaleKey skey = { t, v };
                track.scales.push_back(skey);
            }
        }
        if(!raw_anim.Validate()) {
            LOG_WARN("Animation " << ai_anim->mName.C_Str() << " failed to validate");
            continue;
        } else {
            LOG("Animation " << ai_anim->mName.C_Str() << " is valid");
        }
        if(is_additive) {
            LOG("Loading " << anim->Name() << " as additive animation");
            ozz::animation::offline::AdditiveAnimationBuilder builder;
            ozz::animation::offline::RawAnimation raw_anim_add;
            if(!builder(raw_anim, &raw_anim_add)) {
                LOG_WARN("Failed to build additive raw animation");
            }

            ozz::animation::offline::AnimationBuilder builder2;
            anim->anim = builder2(raw_anim_add);
        } else {
            ozz::animation::offline::AnimationBuilder builder;
            anim->anim = builder(raw_anim);
        }
        anim->root_motion_node = root_motion_node_name;

        scene->addLocalResource(anim);
    }
}

inline void resourcesFromAssimpScene(
    const aiScene* ai_scene, 
    Scene* scene, 
    std::shared_ptr<Skeleton> skeleton,
    AssetParams& asset_params
) {
    meshesFromAssimpScene(ai_scene, scene, skeleton, asset_params);
    animationsFromAssimpScene(ai_scene, scene, skeleton, asset_params);
}

inline void objectFromAssimpNode(
    const aiScene* ai_scene, 
    aiNode* node, 
    SceneObject* root, 
    SceneObject* object, 
    size_t base_mesh_index, 
    std::vector<std::function<void(void)>>& tasks,
    std::shared_ptr<Skeleton> skeleton
) {
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        objectFromAssimpNode(ai_scene, node->mChildren[i], root, object->createChild(), base_mesh_index, tasks, skeleton);
    }

    object->setName(node->mName.C_Str());
    object->get<Transform>()->setTransform(gfxm::transpose(*(gfxm::mat4*)&node->mTransformation));

    if(node->mNumMeshes > 0) {
        Model* m = object->get<Model>();
        m->mesh = object->getScene()->getLocalResource<Mesh>(base_mesh_index + node->mMeshes[0]);
        // TODO: Set material
        aiMesh* ai_mesh = ai_scene->mMeshes[node->mMeshes[0]];
        if(ai_mesh->mNumBones) {
            Skin* skin = object->get<Skin>();
            for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                aiBone* bone = ai_mesh->mBones[j];
                std::string name(bone->mName.data, bone->mName.length);

                tasks.emplace_back([skin, bone, name, object, root, skeleton](){
                    SceneObject* so = root->findObject(name);
                    if(so) {
                        skin->bones.emplace_back(so->get<Transform>()->getId());
                        skin->bind_pose.emplace_back(gfxm::transpose(*(gfxm::mat4*)&bone->mOffsetMatrix));
                        // TODO: Refactor?
                        skeleton->addBone(bone->mName.C_Str(), *(gfxm::mat4*)&bone->mOffsetMatrix);
                    }
                });
            }
        }
    }
}

inline bool sceneFromFbx(const std::string& filename, Scene* scene, SceneObject* root_node = 0) {
    AssetParams asset_params;
    asset_params.load(filename + ".asset_params");
    
    Assimp::Importer importer;
    const aiScene* ai_scene = importer.ReadFile(
        filename,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_LimitBoneWeights |
        aiProcess_GlobalScale |
        aiProcess_GenUVCoords
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

    SceneObject* root = root_node == 0 ? scene->getRootObject() : root_node;
    if(ai_scene->mMetaData) {
        double scaleFactor = 1.0;
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0f;
            scaleFactor *= 0.01;
            root->get<Transform>()->scale((float)scaleFactor);
            if(root->getParent()) {
                gfxm::vec3 a = root->get<Transform>()->scale();
                gfxm::vec3 b = root->getParent()->get<Transform>()->scale();
                root->get<Transform>()->scale(gfxm::vec3(
                    a.x / b.x, a.y / b.y, a.z / b.z
                ));
            }
        } else {
            // TODO ?
        }
    }
    auto base_mesh_index = scene->localResourceCount<Mesh>();
    auto base_anim_index = scene->localResourceCount<Animation>();

    std::vector<std::function<void(void)>> deferred_tasks;

    std::shared_ptr<Skeleton> skeleton(new Skeleton());
    skeleton->Storage(Resource::LOCAL);
    skeleton->Name("Skeleton");
    scene->addLocalResource(skeleton);

    resourcesFromAssimpScene(ai_scene, scene, skeleton, asset_params);

    for(unsigned i = 0; i < ai_rootNode->mNumChildren; ++i) {
        objectFromAssimpNode(
            ai_scene, 
            ai_rootNode->mChildren[i],
            root, 
            root->createChild(), 
            base_mesh_index, 
            deferred_tasks,
            skeleton
        );
    }

    if(ai_scene->mNumAnimations > 0) {
        auto animator = root->get<Animator>();
        if(skeleton->buildOzzSkeleton()) {
            animator->setSkeleton(skeleton);
        }
    }

    for(unsigned int i = 0; i < ai_scene->mNumAnimations; ++i) {
        auto animator = root->get<Animator>();
        animator->addAnim(scene->getLocalResource<Animation>(base_anim_index + i));
    }

    for(auto& t : deferred_tasks) {
        t();
    }

    std::vector<std::string> tokens = split(filename, '\\');
    if(!tokens.empty()) {
        std::string name = tokens[tokens.size() - 1];
        tokens = split(name, '.');
        name = name.substr(0, name.find_last_of("."));
        root->setName(name);
    } else {
        root->setName("object");
    }

    LOG("Fbx skeleton size: " << skeleton->boneCount());

    return true;
}

#endif
