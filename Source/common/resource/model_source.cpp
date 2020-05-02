#include "model_source.hpp"

#include "../util/assimp_scene.hpp"

#include "../util/filesystem.hpp"

#include "../attributes/action_state_machine.hpp"

void ModelSource::loadSkeleton(const aiScene* ai_scene) {
    skeleton.reset(new Skeleton());

    unsigned int mesh_count = ai_scene->mNumMeshes;
    for(unsigned int i = 0; i < mesh_count; ++i) {
        auto ai_mesh = ai_scene->mMeshes[i];
        for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
            unsigned int bone_index = j;
            aiBone* bone = ai_mesh->mBones[j];
            skeleton->addBone(bone->mName.C_Str());
        }
    }
    for(unsigned i = 0; i < ai_scene->mNumAnimations; ++i) {
        aiAnimation* ai_anim = ai_scene->mAnimations[i];
        for(unsigned j = 0; j < ai_anim->mNumChannels; ++j) {
            aiNodeAnim* ai_node_anim = ai_anim->mChannels[j];
            skeleton->addBone(ai_node_anim->mNodeName.C_Str());
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
    finalizeBone(ai_scene->mRootNode, skeleton);
}

static std::shared_ptr<Mesh> mergeMeshes(const std::vector<const aiMesh*>& ai_meshes) {
    std::shared_ptr<Mesh> mesh;
    mesh.reset(new Mesh());

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

    size_t uv_layer_count = 0;
    for(auto ai_m : ai_meshes) {
        for(unsigned j = 0; j < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++j) {
            if(!ai_m->HasTextureCoords(j)) break;
            if(uv_layer_count == j) uv_layer_count++;
        }
    }
    uv_layers.resize(uv_layer_count);

    uint32_t indexOffset = 0;
    uint32_t baseIndexValue = 0;
    for(auto ai_m : ai_meshes) {
        int32_t vertexCount = ai_m->mNumVertices;
        int32_t triCount = ai_m->mNumFaces;
        uint32_t indexCount = triCount * 3;
        
        std::vector<float> tangent_;
        std::vector<float> bitangent_;
        std::vector<gfxm::vec4> boneIndices_;
        std::vector<gfxm::vec4> boneWeights_;

        mesh->submeshes.emplace_back(
            Mesh::SubMesh {
                indexOffset * sizeof(uint32_t),
                indexCount
            }
        );

        for(unsigned j = 0; j < vertexCount; ++j) {
            aiVector3D& v = ai_m->mVertices[j];
            vertices.emplace_back(v.x);
            vertices.emplace_back(v.y);
            vertices.emplace_back(v.z);
        }
        for(unsigned j = 0; j < triCount; ++j) {
            aiFace& f = ai_m->mFaces[j];
            for(unsigned k = 0; k < f.mNumIndices; ++k) {
                indices.emplace_back(baseIndexValue + f.mIndices[k]);
            }
        }
        for(unsigned j = 0; j < vertexCount; ++j) {
            aiVector3D& n = ai_m->mNormals[j];
            normal_layers[0].emplace_back(n.x);
            normal_layers[0].emplace_back(n.y);
            normal_layers[0].emplace_back(n.z);
        }
        for(unsigned j = 0; j < uv_layer_count; ++j) {
            if(ai_m->HasTextureCoords(j)) {
                std::vector<float> uv_layer;
                for(unsigned k = 0; k < vertexCount; ++k) {
                    aiVector3D& uv = ai_m->mTextureCoords[j][k];
                    uv_layer.emplace_back(uv.x);
                    uv_layer.emplace_back(uv.y);
                }
                uv_layers[j].insert(uv_layers[j].end(), uv_layer.begin(), uv_layer.end());
            } else {
                std::vector<float> dummy_uv;
                dummy_uv.resize(vertexCount * 2);
                uv_layers[j].insert(uv_layers[j].end(), dummy_uv.begin(), dummy_uv.end());
            }
        }

        if(ai_m->mNumBones) {
            boneIndices_.resize(vertexCount);
            boneWeights_.resize(vertexCount);
            for(unsigned j = 0; j < ai_m->mNumBones; ++j) {
                unsigned int bone_index = j;
                aiBone* bone = ai_m->mBones[j];
                
                for(unsigned k = 0; k < bone->mNumWeights; ++k) {
                    aiVertexWeight& w = bone->mWeights[k];
                    gfxm::vec4& indices_ref = boneIndices_[w.mVertexId];
                    gfxm::vec4& weights_ref = boneWeights_[w.mVertexId];
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
        boneIndices.insert(boneIndices.end(), boneIndices_.begin(), boneIndices_.end());
        boneWeights.insert(boneWeights.end(), boneWeights_.begin(), boneWeights_.end());

        tangent_.resize(vertexCount * 3);
        bitangent_.resize(vertexCount * 3);
        if(ai_m->mTangents && ai_m->mBitangents) {
            for(unsigned j = 0; j < vertexCount; ++j) {
                aiVector3D& n = ai_m->mTangents[j];
                tangent_[j * 3] = (n.x);
                tangent_[j * 3 + 1] = (n.y);
                tangent_[j * 3 + 2] = (n.z);
            }
            for(unsigned j = 0; j < vertexCount; ++j) {
                aiVector3D& n = ai_m->mBitangents[j];
                bitangent_[j * 3] = (n.x);
                bitangent_[j * 3 + 1] = (n.y);
                bitangent_[j * 3 + 2] = (n.z);
            }
        }
        tangent.insert(tangent.end(), tangent_.begin(), tangent_.end());
        bitangent.insert(bitangent.end(), bitangent_.begin(), bitangent_.end());

        indexOffset += indexCount;
        baseIndexValue += vertexCount;
    }

    mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(float));
    if(normal_layers.size() > 0) {
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Normal, normal_layers[0].data(), normal_layers[0].size() * sizeof(float));
    }
    if(uv_layers.size() > 0) {
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float));
    }
    if(!boneIndices.empty() && !boneWeights.empty()) {
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::BoneIndex4, boneIndices.data(), boneIndices.size() * sizeof(gfxm::vec4));
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::BoneWeight4, boneWeights.data(), boneWeights.size() * sizeof(gfxm::vec4));
    }
    mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Tangent, tangent.data(), tangent.size() * sizeof(float));
    mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Bitangent, bitangent.data(), bitangent.size() * sizeof(float));

    mesh->mesh.setIndices(indices.data(), indices.size());

    mesh->makeAabb();

    return mesh;
}

static std::shared_ptr<Mesh> loadMesh(aiMesh* ai_mesh) {
    // TODO: Preload before building to retain extra data (?)
    std::shared_ptr<Mesh> mesh_ref;
    mesh_ref.reset(new Mesh());

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

    mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(float));
    if(normal_layers.size() > 0) {
        mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Normal, normal_layers[0].data(), normal_layers[0].size() * sizeof(float));
    }
    if(uv_layers.size() > 0) {
        mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float));
    }
    if(!boneIndices.empty() && !boneWeights.empty()) {
        mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::BoneIndex4, boneIndices.data(), boneIndices.size() * sizeof(gfxm::vec4));
        mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::BoneWeight4, boneWeights.data(), boneWeights.size() * sizeof(gfxm::vec4));
    }
    mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Tangent, tangent.data(), tangent.size() * sizeof(float));
    mesh_ref->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Bitangent, bitangent.data(), bitangent.size() * sizeof(float));

    mesh_ref->mesh.setIndices(indices.data(), indices.size());

    return mesh_ref;
}

void ModelSource::loadMeshes(const aiScene* ai_scene) {
    unsigned int mesh_count = ai_scene->mNumMeshes;
    for(unsigned int i = 0; i < mesh_count; ++i) {
        meshes.emplace_back(loadMesh(ai_scene->mMeshes[i]));
    }
}

static void extractRootMotionFromAssimpAnimNode(
    aiNodeAnim* ai_node_anim, 
    AnimNode& node, 
    AnimNode& rm_node, 
    Skeleton* skeleton,
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

static void animNodeFromAssimpNode(aiNodeAnim* ai_node_anim, AnimNode& node) {
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

static std::shared_ptr<Animation> loadAnim(aiAnimation* ai_anim, Skeleton* skeleton) {
    // TODO: Preload before building to retain extra data (?)
    std::shared_ptr<Animation> anim;
    anim.reset(new Animation());

    double fps = ai_anim->mTicksPerSecond;
    double len = ai_anim->mDuration;

    //AssetParams& anim_asset_params = 
    //    asset_params.get_object("Animation").get_object(ai_anim->mName.C_Str());
    bool is_additive = false;//anim_asset_params.get_bool("Additive", false);
    bool enable_root_motion = false;//anim_asset_params.get_bool("EnableRootMotion", false);
    std::string root_motion_node_name = "";//anim_asset_params.get_string("RootMotionNode", "");
    bool root_rotation = false;//anim_asset_params.get_bool("RootRotation", false);
    bool root_translation = false;//anim_asset_params.get_bool("RootTranslation", false);

    anim->clearNodes();
    anim->Storage(Resource::GLOBAL);
    anim->Name(MKSTR(replace_reserved_chars(ai_anim->mName.C_Str(), '_') << ".anm"));

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

    return anim;
}

void ModelSource::loadAnims(const aiScene* ai_scene) {
    for(unsigned i = 0; i < ai_scene->mNumAnimations; ++i) {
        anims.emplace_back(loadAnim(ai_scene->mAnimations[i], skeleton.get()));
    }
}

void ModelSource::loadResources(const aiScene* ai_scene) {
    loadSkeleton(ai_scene);
    //loadMeshes(ai_scene);
    loadAnims(ai_scene);

    for(unsigned i = 0; i < ai_scene->mNumMaterials; ++i) {
        std::shared_ptr<Material> mat;
        mat.reset(new Material());
        aiMaterial* ai_mat = ai_scene->mMaterials[i];
        mat->Name(MKSTR(ai_mat->GetName().C_Str() << ".mat"));
        aiString path;
        if(ai_mat->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
            std::string tex_path = path.data;
            LOG(tex_path);
            //mat->albedo = retrieve<Texture2D>(tex_path);
        }
        if(ai_mat->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
            std::string tex_path = path.data;
            LOG(tex_path);
        }
        if(ai_mat->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS) {
            std::string tex_path = path.data;
            LOG(tex_path);
        }
        materials.emplace_back(mat);
    }
}

void ModelSource::loadSceneGraph(const aiScene* ai_scene, aiNode* node, ktNode* o) {
    o->setName(node->mName.C_Str());
    o->getTransform()->setTransform(
        gfxm::transpose(*(gfxm::mat4*)&node->mTransformation)
    );
    
    for(unsigned i = 0; i < node->mNumChildren; ++i) {
        auto c = o->createChild();
        loadSceneGraph(ai_scene, node->mChildren[i], c);
    }

    if(node->mNumMeshes) {
        auto m = o->get<Model>();
        if(m) {
            std::vector<const aiMesh*> ai_meshes;
            for(unsigned i = 0; i < node->mNumMeshes; ++i) {
                ai_meshes.emplace_back(ai_scene->mMeshes[node->mMeshes[i]]);
            }
            std::shared_ptr<Mesh> mesh = mergeMeshes(ai_meshes);
            
            mesh->Name(MKSTR(node->mName.C_Str() << ".msh"));
            meshes.emplace_back(mesh);
            for(unsigned i = 0; i < node->mNumMeshes; ++i) {
                auto& seg = m->getSegment(i);
                seg.mesh = mesh;
                seg.material = materials[ai_scene->mMeshes[node->mMeshes[i]]->mMaterialIndex];
                seg.submesh_index = i;
                aiMesh* ai_mesh = ai_scene->mMeshes[node->mMeshes[i]];
                if(ai_mesh->mNumBones) {
                    seg.skin_data.reset(new Model::SkinData());
                    for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                        aiBone* ai_bone = ai_mesh->mBones[j];
                        std::string name(ai_bone->mName.data, ai_bone->mName.length);

                        ktNode* bone_object = o->getRoot()->findObject(name);
                        if(bone_object) {
                            seg.skin_data->bone_nodes.emplace_back(bone_object);
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

void ModelSource::loadSceneGraph(const aiScene* ai_scene) {
    loadSceneGraph(ai_scene, ai_scene->mRootNode, scene->getRoot());
}

ModelSource::ModelSource() {
    scene.reset(new GameScene());
}
ModelSource::~ModelSource() {

}

bool ModelSource::deserialize(in_stream& in, size_t sz) {
    std::vector<char> buf;
    buf.resize(sz);
    in.read(buf.data(), sz);
    AssimpScene scn(buf.data(), sz, this->Name());
    const aiScene* ai_scene = scn.getScene();
    if(!ai_scene) {
        LOG_WARN("Assimp import: failed to load scene '" << Name() << "'");
        return false;
    }
    aiNode* ai_rootNode = ai_scene->mRootNode;
    if(!ai_rootNode) {
        LOG_WARN("Assimp import: No root node '" << Name() << "'");
        return false;
    }
    double scaleFactor = 1.0f;
    if(ai_scene->mMetaData) {
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0f;
            scaleFactor *= 0.01;
        }
    }

    std::string scene_name;
    scene_name = Name().substr(Name().find_last_of("/") + 1);
    scene_name = scene_name.substr(0, scene_name.find_first_of('.')); 

    loadResources(ai_scene);
    loadSceneGraph(ai_scene);
    scene->getRoot()->setName(scene_name);
    scene->getRoot()->getTransform()->setScale((float)scaleFactor);

/*
    if(anims.size() > 0) {
        auto anim_stack = scene->getRoot()->get<AnimationStack>();
        anim_stack->setSkeleton(skeleton);
    }
    for(unsigned i = 0; i < anims.size(); ++i) {
        auto anim_stack = scene->getRoot()->get<AnimationStack>();
        anim_stack->addAnim(anims[i]);
    }
*/
    scene->getRoot()->refreshAabb();

    scene->Name(scene_name + ".so");
    if(skeleton) {
        skeleton->scale_factor = (float)scaleFactor;
        skeleton->Name(scene_name + ".skl");
        scene->getRoot()->get<SkeletonRef>()->skeleton = skeleton;
    }
    return true;
}

bool ModelSource::unpack(const std::string& dir) {
    std::string rname = Name();
    rname = rname.substr(0, rname.find_last_of('.'));

    const std::string res_dir_rel = rname.substr(0, rname.find_last_of("/"));
    const std::string res_dir_dep_rel = rname;

    const std::string unpack_dir = dir + "/" + rname;
    const std::string mesh_dir = unpack_dir + "/mesh";
    const std::string anim_dir = unpack_dir + "/anim";
    const std::string material_dir = unpack_dir + "/material";
    const std::string texture_dir = unpack_dir + "/texture";
    const std::string source_dir = unpack_dir.substr(0, unpack_dir.find_last_of("/"));

    createDirRecursive(mesh_dir);
    createDirRecursive(anim_dir);
    createDirRecursive(material_dir);
    createDirRecursive(texture_dir);

    if(skeleton) {
        skeleton->write_to_file(unpack_dir + "/" + skeleton->Name());
        skeleton->Name(res_dir_dep_rel + "/" + skeleton->Name());
    }

    for(auto m : meshes) {
        m->write_to_file(mesh_dir + "/" + m->Name());
        m->Name(res_dir_dep_rel + "/mesh/" + m->Name());
    }
    for(auto a : anims) {
        a->write_to_file(anim_dir + "/" + a->Name());
        a->Name(res_dir_dep_rel + "/anim/" + a->Name());
    }
    for(auto m : materials) {
        m->write_to_file(material_dir + "/" + m->Name());
        m->Name(res_dir_dep_rel + "/material/" + m->Name());
    }
    for(auto t : textures) {
        t->write_to_file(texture_dir + "/" + t->Name());
        t->Name(res_dir_dep_rel + "/texture/" + t->Name());
    }

    if(scene) {
        scene->write_to_file(source_dir + "/" + scene->Name());
    }

    gResourceTree.scanFilesystem(get_module_dir() + "/" + platformGetConfig().data_dir);

    return true;
}
