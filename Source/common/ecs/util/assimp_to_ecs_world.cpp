#include "assimp_to_ecs_world.hpp"

#include "../../util/filesystem.hpp"
#include "../../platform/platform.hpp"
#include "../../util/log.hpp"
#include "../../util/assimp_scene.hpp"
#include "../attribs/transform.hpp"
#include "../attribs/base_attribs.hpp"


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

    mesh->mesh.setAttribData(gl::POSITION, vertices.data(), vertices.size() * sizeof(float));
    if(normal_layers.size() > 0) {
        mesh->mesh.setAttribData(gl::NORMAL, normal_layers[0].data(), normal_layers[0].size() * sizeof(float));
    }
    if(uv_layers.size() > 0) {
        mesh->mesh.setAttribData(gl::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float));
    }
    if(!boneIndices.empty() && !boneWeights.empty()) {
        mesh->mesh.setAttribData(gl::BONE_INDEX4, boneIndices.data(), boneIndices.size() * sizeof(gfxm::vec4));
        mesh->mesh.setAttribData(gl::BONE_WEIGHT4, boneWeights.data(), boneWeights.size() * sizeof(gfxm::vec4));
    }
    mesh->mesh.setAttribData(gl::TANGENT, tangent.data(), tangent.size() * sizeof(float));
    mesh->mesh.setAttribData(gl::BITANGENT, bitangent.data(), bitangent.size() * sizeof(float));

    mesh->mesh.setIndices(indices.data(), indices.size());

    mesh->makeAabb();

    return mesh;
}


typedef std::map<std::string, entity_id> name_to_ent_map_t;

void assimpImportEcsSceneGraph(ecsysSceneGraph* graph, name_to_ent_map_t& node_map, const aiScene* ai_scene, aiNode* ai_node, entity_id ent) {
    ecsName* ecs_name = graph->getWorld()->getAttrib<ecsName>(ent);
    ecsTRS* ecs_trs = graph->getWorld()->getAttrib<ecsTRS>(ent);
    ecs_name->name = ai_node->mName.C_Str();
    ecs_trs->fromMatrix(
        gfxm::transpose(*(gfxm::mat4*)&ai_node->mTransformation)
    );
    graph->getWorld()->getAttrib<ecsWorldTransform>(ent);
    
    node_map[ecs_name->name] = ent;

    for(unsigned i = 0; i < ai_node->mNumChildren; ++i) {
        auto child_ent = graph->createNode();
        graph->setParent(child_ent, ent);
        assimpImportEcsSceneGraph(graph, node_map, ai_scene, ai_node->mChildren[i], child_ent);
    }

    if(ai_node->mNumMeshes) {
        auto ecs_meshes = graph->getWorld()->getAttrib<ecsMeshes>(ent);

        std::vector<const aiMesh*> ai_meshes;
        for(unsigned i = 0; i < ai_node->mNumMeshes; ++i) {
            ai_meshes.emplace_back(ai_scene->mMeshes[ai_node->mMeshes[i]]);
        }
        std::shared_ptr<Mesh> mesh = mergeMeshes(ai_meshes);
        
        for(unsigned i = 0; i < ai_node->mNumMeshes; ++i) {
            auto& seg = ecs_meshes->getSegment(i);
            seg.mesh = mesh;
            // TODO: Assign material
            seg.submesh_index = i;
            
            aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
            if(ai_mesh->mNumBones) {
                seg.skin_data.reset(new ecsMeshes::SkinData());
                for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                    aiBone* ai_bone = ai_mesh->mBones[j];
                    std::string name(ai_bone->mName.data, ai_bone->mName.length);

                    auto it = node_map.find(name);
                    if(it != node_map.end()) {
                        seg.skin_data->bone_nodes.emplace_back(graph->getWorld()->getAttrib<ecsWorldTransform>(it->second));
                        seg.skin_data->bind_transforms.emplace_back(
                            gfxm::transpose(*(gfxm::mat4*)&ai_bone->mOffsetMatrix)
                        );
                    }
                }
            }
        }
    }
}

void assimpImportEcsSceneGraph(ecsysSceneGraph* graph, const aiScene* ai_scene) {
    entity_id root_ent = graph->createNode();
    name_to_ent_map_t node_map;
    assimpImportEcsSceneGraph(graph, node_map, ai_scene, ai_scene->mRootNode, root_ent);

    double scaleFactor = 1.0f;
    if(ai_scene->mMetaData) {
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0f;
            scaleFactor *= 0.01;
        }
    }
    graph->getWorld()->getAttrib<ecsTRS>(root_ent)->scale = gfxm::vec3((float)scaleFactor, (float)scaleFactor, (float)scaleFactor);
}

bool assimpImportEcsScene(ecsysSceneGraph* graph, AssimpScene* scn) {
    const aiScene* ai_scene = scn->getScene();
    if(!ai_scene) {
        LOG_WARN("Assimp import: failed to load scene '" << "TODO: PUT NAME HERE" << "'");
        return false;
    }
    aiNode* ai_rootNode = ai_scene->mRootNode;
    if(!ai_rootNode) {
        LOG_WARN("Assimp import: No root node '" << "TODO: PUT NAME HERE" << "'");
        return false;
    }

    //std::string scene_name;
    //scene_name = Name().substr(Name().find_last_of("/") + 1);
    //scene_name = scene_name.substr(0, scene_name.find_first_of('.')); 

    //loadResources(ai_scene);
    assimpImportEcsSceneGraph(graph, ai_scene);
    //scene->getRoot()->setName(scene_name);

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
    return true;
}


bool assimpImportEcsScene(ecsysSceneGraph* graph, const char* filename) {
    std::ifstream f(get_module_dir() + "/" + platformGetConfig().data_dir + "/" + std::string(filename), std::ios::binary | std::ios::ate);
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

    AssimpScene assimp_scene(buffer.data(), buffer.size(), filename);

    return assimpImportEcsScene(graph, &assimp_scene);
}