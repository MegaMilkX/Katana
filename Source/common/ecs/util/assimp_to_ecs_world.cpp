#include "assimp_to_ecs_world.hpp"

#include "../../util/filesystem.hpp"
#include "../../platform/platform.hpp"
#include "../../util/log.hpp"
#include "../../util/assimp_scene.hpp"
#include "../attribs/transform.hpp"
#include "../attribs/base_attribs.hpp"

#include "../../lib/xatlas/xatlas.h"

#include "../../util/progress_counter.hpp"

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
    std::vector<std::vector<uint8_t>> rgb_layers;
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

    size_t rgb_layer_count = 0;
    for(auto ai_m : ai_meshes) {
        for(unsigned j = 0; j < AI_MAX_NUMBER_OF_COLOR_SETS; ++j) {
            if(!ai_m->HasVertexColors(j)) break;
            if(rgb_layer_count == j) rgb_layer_count++;
        }
    }
    rgb_layers.resize(rgb_layer_count);

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
        for(unsigned j = 0; j < rgb_layer_count; ++j) {
            if(ai_m->HasVertexColors(j)) {
                std::vector<uint8_t> rgb_layer;
                for(unsigned k = 0; k < vertexCount; ++k) {
                    aiColor4D& rgb = ai_m->mColors[j][k];
                    rgb_layer.emplace_back(255 * rgb.r);
                    rgb_layer.emplace_back(255 * rgb.g);
                    rgb_layer.emplace_back(255 * rgb.b);
                    rgb_layer.emplace_back(255 * rgb.a);
                }
                rgb_layers[j].insert(rgb_layers[j].end(), rgb_layer.begin(), rgb_layer.end());
            } else {
                std::vector<uint8_t> dummy_rgb(vertexCount * 4, 255);
                rgb_layers[j].insert(rgb_layers[j].end(), dummy_rgb.begin(), dummy_rgb.end());
            }
        }
        if (rgb_layer_count == 0) {
            rgb_layers.push_back(std::vector<uint8_t>(vertexCount * 4, 255));
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
        for(auto& bw : boneWeights_) {
            float total = bw.x + bw.y + bw.z + bw.w;
            if(total == .0f) {
                continue;
            }
            bw = bw / total;
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

    
    // Generate cool uv map TODO: Remove 
    /*
    {
        xatlas::Atlas* atlas = xatlas::Create();

        xatlas::MeshDecl meshDecl = { 0 };
        meshDecl.vertexCount = vertices.size() / 3;
        meshDecl.indexCount = indices.size();
        meshDecl.vertexPositionData = vertices.data();
        meshDecl.vertexPositionStride = sizeof(float) * 3;
        meshDecl.indexData = indices.data();
        meshDecl.indexFormat = xatlas::IndexFormat::UInt32;

        auto ret = xatlas::AddMesh(atlas, meshDecl);
        assert(ret == xatlas::AddMeshError::Success);

        xatlas::ComputeCharts(atlas);
        xatlas::ParameterizeCharts(atlas);
        xatlas::PackCharts(atlas);

        std::vector<float> new_verts;
        std::vector<uint32_t> new_indices;
        std::vector<std::vector<float>> new_normal_layers;
        new_normal_layers.resize(normal_layers.size());
        std::vector<float> new_tangent;
        std::vector<float> new_bitangent;
        std::vector<std::vector<float>> new_uv_layers;
        new_uv_layers.resize(uv_layers.size());
        std::vector<std::vector<uint8_t>> new_rgb_layers;
        new_rgb_layers.resize(rgb_layers.size());
        std::vector<gfxm::vec4> new_boneIndices;
        std::vector<gfxm::vec4> new_boneWeights;
        std::vector<float> xuv_layer;
        int offset = 0;
        for(int i = 0; i < atlas->meshCount; ++i) {
            int xvertexCount = atlas->meshes[i].vertexCount;
            int xindexCount = atlas->meshes[i].indexCount;
            // Alloc buffer space
            new_verts.resize(new_verts.size() + xvertexCount * 3);
            for(int j = 0; j < new_normal_layers.size(); ++j) {
                new_normal_layers[j].resize(new_normal_layers.size() + xvertexCount * 3);
            }
            new_tangent.resize(new_tangent.size() + xvertexCount * 3);
            new_bitangent.resize(new_bitangent.size() + xvertexCount * 3);
            for(int j = 0; j < new_uv_layers.size(); ++j) {
                new_uv_layers[j].resize(new_uv_layers.size() + xvertexCount * 2);
            }
            for(int j = 0; j < new_rgb_layers.size(); ++j) {
                new_rgb_layers[j].resize(new_rgb_layers.size() + xvertexCount * 4);
            }
            if (boneIndices.size() && boneWeights.size()) {
                new_boneIndices.resize(new_boneIndices.size() + xvertexCount * 4);
                new_boneWeights.resize(new_boneWeights.size() + xvertexCount * 4);
            }
            xuv_layer.resize(xuv_layer.size() + xvertexCount * 2);

            // Copy data
            for (int j = 0; j < xindexCount; ++j) {
                new_indices.push_back(offset + atlas->meshes[i].indexArray[j]);
            }
            xatlas::Vertex* xvertices = atlas->meshes[i].vertexArray;
            for(int j = 0; j < xvertexCount; ++j) {
                xuv_layer[offset + 2 * j] = xvertices[j].uv[0] / atlas->width;
                xuv_layer[offset + 2 * j + 1] = xvertices[j].uv[1] / atlas->height;

                memcpy(&new_verts[offset + 3 * j], &vertices[xvertices[j].xref * 3], sizeof(float) * 3);
                for(int k = 0; k < new_normal_layers.size(); ++k) {
                    memcpy(&new_normal_layers[k][offset + 3 * j], &normal_layers[k][xvertices[j].xref * 3], sizeof(float) * 3);
                }
                memcpy(&new_tangent[offset + 3 * j], &tangent[xvertices[j].xref * 3], sizeof(float) * 3);
                memcpy(&new_bitangent[offset + 3 * j], &bitangent[xvertices[j].xref * 3], sizeof(float) * 3);
                for(int k = 0; k < new_uv_layers.size(); ++k) {
                    memcpy(&new_uv_layers[k][offset + 2 * j], &uv_layers[k][xvertices[j].xref * 2], sizeof(float) * 2);
                }
                for(int k = 0; k < new_rgb_layers.size(); ++k) {
                    memcpy(&new_rgb_layers[k][offset + 4 * j], &rgb_layers[k][xvertices[j].xref * 4], sizeof(uint8_t) * 4);
                }
                if (boneIndices.size() && boneWeights.size()) {
                    memcpy(&new_boneIndices[offset + 4 * j], &boneIndices[xvertices[j].xref * 4], sizeof(float) * 4);
                    memcpy(&new_boneWeights[offset + 4 * j], &boneWeights[xvertices[j].xref * 4], sizeof(float) * 4);
                }
            }

            offset += atlas->meshes[i].vertexCount * 3;
        }

        xatlas::Destroy(atlas);

        std::swap(indices, new_indices);
        std::swap(vertices, new_verts);
        for(int j = 0; j < new_normal_layers.size(); ++j) {
            std::swap(normal_layers[j], new_normal_layers[j]);
        }
        std::swap(tangent, new_tangent);
        std::swap(bitangent, new_bitangent);
        for(int j = 0; j < new_uv_layers.size(); ++j) {
            std::swap(uv_layers[j], new_uv_layers[j]);
        }
        for(int j = 0; j < new_rgb_layers.size(); ++j) {
            std::swap(rgb_layers[j], new_rgb_layers[j]);
        }
        std::swap(boneIndices, new_boneIndices);
        std::swap(boneWeights, new_boneWeights);

        if (uv_layers.empty()) {
            uv_layers.emplace_back(xuv_layer);
        } else {
            std::swap(uv_layers[0], xuv_layer);
        }

    }*/

    mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(float));
    if(normal_layers.size() > 0) {
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::Normal, normal_layers[0].data(), normal_layers[0].size() * sizeof(float));
    }
    if(uv_layers.size() > 0) {
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float));
    }
    if(rgb_layers.size() > 0) {
        mesh->mesh.setAttribData(VERTEX_FMT::ENUM_GENERIC::ColorRGBA, rgb_layers[0].data(), rgb_layers[0].size() * sizeof(uint8_t));
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


typedef std::map<std::string, entity_id>        name_to_ent_map_t;
typedef std::unordered_map<aiNode*, entity_id>  node_to_entity_t;
#include "../util/threading/delegated_call.hpp"
void assimpImportEcsSceneGraph(ecsWorld* world, name_to_ent_map_t& node_map, node_to_entity_t& deferred_entity_map, const aiScene* ai_scene, aiNode* ai_node, ecsEntityHandle ent) {
    ecsName* ecs_name = ent.getAttrib<ecsName>();
    ecsTranslation* translation = ent.getAttrib<ecsTranslation>();
    ecsRotation* rotation = ent.getAttrib<ecsRotation>();
    ecsScale* scale = ent.getAttrib<ecsScale>();

    ecs_name->name = ai_node->mName.C_Str();
    
    if(ai_scene->mRootNode != ai_node) {
        aiVector3D ai_pos;
        aiQuaternion ai_quat;
        aiVector3D ai_scale;
        ai_node->mTransformation.Decompose(ai_scale, ai_quat, ai_pos);
        
        translation->setPosition(ai_pos.x, ai_pos.y, ai_pos.z);
        rotation->setRotation(ai_quat.x, ai_quat.y, ai_quat.z, ai_quat.w);
        scale->setScale(ai_scale.x, ai_scale.y, ai_scale.z);
    }

    ent.getAttrib<ecsWorldTransform>();
    
    node_map[ecs_name->name] = ent.getId();

    for(unsigned i = 0; i < ai_node->mNumChildren; ++i) {
        auto child_ent = world->createEntity();
        world->setParent(ent.getId(), child_ent.getId());
        assimpImportEcsSceneGraph(world, node_map, deferred_entity_map, ai_scene, ai_node->mChildren[i], child_ent);
    }

    if(ai_node->mNumMeshes) {
        deferred_entity_map[ai_node] = ent.getId();
    }
}

void assimpImportEcsSceneMeshes(ecsWorld* world, name_to_ent_map_t& node_map, node_to_entity_t& deferred_entity_map, const aiScene* ai_scene) {
    for(auto& it : deferred_entity_map) {
        auto ai_node = it.first;
        auto entity =  it.second;

        ecsEntityHandle ent(world, entity);

        if(ai_node->mNumMeshes) {
            deferred_entity_map[ai_node] = ent.getId();
            auto ecs_meshes = ent.getAttrib<ecsMeshes>();

            std::vector<const aiMesh*> ai_meshes;
            for(unsigned i = 0; i < ai_node->mNumMeshes; ++i) {
                ai_meshes.emplace_back(ai_scene->mMeshes[ai_node->mMeshes[i]]);
            }
            progressStep(1.0f, "Merging meshes");
            std::shared_ptr<Mesh> mesh = mergeMeshes(ai_meshes);
            
            for(unsigned i = 0; i < ai_node->mNumMeshes; ++i) {
                progressStep(1.0f, "Uploading meshes");
                auto& seg = ecs_meshes->getSegment(i);
                seg.mesh = mesh;
                // TODO: Assign material
                seg.submesh_index = i;
                
                aiMesh* ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
                delegatedCall([ai_mesh, &seg, &node_map, world](){
                    if(ai_mesh->mNumBones) {
                        seg.skin_data.reset(new ecsMeshes::SkinData());
                        
                        size_t vertexCount = seg.mesh->vertexCount();
                        seg.skin_data->vao_cache.reset(new gl::VertexArrayObject);
                        seg.skin_data->position_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                        seg.skin_data->normal_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                        seg.skin_data->tangent_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                        seg.skin_data->bitangent_cache.reset(new gl::Buffer(GL_STREAM_DRAW, vertexCount * sizeof(float) * 3));
                        seg.skin_data->vao_cache->attach(seg.skin_data->position_cache->getId(), 
                            VERTEX_FMT::ENUM_GENERIC::Position, VERTEX_FMT::Position::count, 
                            VERTEX_FMT::Position::gl_type, VERTEX_FMT::Position::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->vao_cache->attach(seg.skin_data->normal_cache->getId(), 
                            VERTEX_FMT::ENUM_GENERIC::Normal, VERTEX_FMT::Normal::count, 
                            VERTEX_FMT::Normal::gl_type, VERTEX_FMT::Normal::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->vao_cache->attach(seg.skin_data->tangent_cache->getId(), 
                            VERTEX_FMT::ENUM_GENERIC::Tangent, VERTEX_FMT::Tangent::count, 
                            VERTEX_FMT::Tangent::gl_type, VERTEX_FMT::Tangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->vao_cache->attach(seg.skin_data->bitangent_cache->getId(), 
                            VERTEX_FMT::ENUM_GENERIC::Bitangent, VERTEX_FMT::Bitangent::count, 
                            VERTEX_FMT::Bitangent::gl_type, VERTEX_FMT::Bitangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->pose_cache.reset(new gl::Buffer(GL_STATIC_DRAW, sizeof(float) * 16 * ai_mesh->mNumBones));

                        if(seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UV)) {
                            GLuint id = seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UV)->getId();
                            seg.skin_data->vao_cache->attach(id, VERTEX_FMT::ENUM_GENERIC::UV, 
                                VERTEX_FMT::UV::count, VERTEX_FMT::UV::gl_type, 
                                VERTEX_FMT::UV::normalized ? GL_TRUE : GL_FALSE, 0, 0
                            );
                        }
                        if(seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)) {
                            GLuint id = seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::UVLightmap)->getId();
                            seg.skin_data->vao_cache->attach(id, VERTEX_FMT::ENUM_GENERIC::UVLightmap, 
                                VERTEX_FMT::UVLightmap::count, VERTEX_FMT::UVLightmap::gl_type, 
                                VERTEX_FMT::UVLightmap::normalized ? GL_TRUE : GL_FALSE, 0, 0
                            );
                        }
                        if(seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::ColorRGBA)) {
                            GLuint id = seg.mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::ColorRGBA)->getId();
                            seg.skin_data->vao_cache->attach(id, VERTEX_FMT::ENUM_GENERIC::ColorRGBA, 
                                VERTEX_FMT::ColorRGBA::count, VERTEX_FMT::ColorRGBA::gl_type, 
                                VERTEX_FMT::ColorRGBA::normalized ? GL_TRUE : GL_FALSE, 0, 0
                            );
                        }
                        GLuint index_buf_id = seg.mesh->mesh.getIndexBuffer()->getId();
                        seg.skin_data->vao_cache->attachIndexBuffer(index_buf_id);

                        for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                            aiBone* ai_bone = ai_mesh->mBones[j];
                            std::string name(ai_bone->mName.data, ai_bone->mName.length);

                            auto it = node_map.find(name);
                            if(it != node_map.end()) {
                                seg.skin_data->bone_nodes.emplace_back(world->getAttrib<ecsWorldTransform>(it->second));
                                seg.skin_data->bind_transforms.emplace_back(
                                    gfxm::transpose(*(gfxm::mat4*)&ai_bone->mOffsetMatrix)
                                );
                            }
                        }
                    }
                });
            }
        }
    }
}

entity_id assimpImportEcsSceneGraph(ecsWorld* world, const aiScene* ai_scene) {
    ecsEntityHandle root_ent = world->createEntity();
    name_to_ent_map_t node_map;
    node_to_entity_t deferred_entity_map;
    assimpImportEcsSceneGraph(world, node_map, deferred_entity_map, ai_scene, ai_scene->mRootNode, root_ent);
    assimpImportEcsSceneMeshes(world, node_map, deferred_entity_map, ai_scene);

    double scaleFactor = 1.0f;
    if(ai_scene->mMetaData) {
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0f;
            scaleFactor *= 0.01;
        }
    }
    root_ent.getAttrib<ecsScale>()->setScale((float)scaleFactor);
    return root_ent;
}

entity_id assimpImportEcsScene(ecsWorld* world, AssimpScene* scn) {
    const aiScene* ai_scene = scn->getScene();
    if(!ai_scene) {
        LOG_WARN("Assimp import: failed to load scene '" << "TODO: PUT NAME HERE" << "'");
        return -1;
    }
    aiNode* ai_rootNode = ai_scene->mRootNode;
    if(!ai_rootNode) {
        LOG_WARN("Assimp import: No root node '" << "TODO: PUT NAME HERE" << "'");
        return -1;
    }

    //std::string scene_name;
    //scene_name = Name().substr(Name().find_last_of("/") + 1);
    //scene_name = scene_name.substr(0, scene_name.find_first_of('.')); 

    //loadResources(ai_scene);
    entity_id e = assimpImportEcsSceneGraph(world, ai_scene);
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
    return e;
}


entity_id assimpImportEcsScene(ecsWorld* world, const char* filename) {
    progressBegin("Fbx import", 100);

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

    progressStep(1.0f, "Reading fbx scene");
    AssimpScene assimp_scene(buffer.data(), buffer.size(), filename);
    progressStep(1.0f, "Constructing world");
    entity_id e = assimpImportEcsScene(world, &assimp_scene);
    
    progressEnd();
    return e;
}