#include "assimp_to_ecs_world.hpp"

#include "../../util/filesystem.hpp"
#include "../../platform/platform.hpp"
#include "../../util/log.hpp"
#include "../../util/import/assimp.hpp"
#include "../attribs/transform.hpp"
#include "../attribs/base_attribs.hpp"

#include "../../lib/xatlas/xatlas.h"

#include "../../util/progress_counter.hpp"


typedef std::map<std::string, entity_id>        name_to_ent_map_t;
typedef std::unordered_map<aiNode*, entity_id>  node_to_entity_t;
#include "../util/threading/delegated_call.hpp"
void assimpImportEcsSceneGraph(ecsWorld* world, name_to_ent_map_t& node_map, node_to_entity_t& deferred_entity_map, const aiScene* ai_scene, aiNode* ai_node, ecsEntityHandle ent) {
    ent.getAttrib<ecsName>();
    ent.getAttrib<ecsTranslation>();
    ent.getAttrib<ecsRotation>();
    ent.getAttrib<ecsScale>();

    ent.getAttrib<ecsName>()->name = ai_node->mName.C_Str();
    
    if(ai_scene->mRootNode != ai_node) {
        aiVector3D ai_pos;
        aiQuaternion ai_quat;
        aiVector3D ai_scale;
        ai_node->mTransformation.Decompose(ai_scale, ai_quat, ai_pos);
        
        ent.getAttrib<ecsTranslation>()->setPosition(ai_pos.x, ai_pos.y, ai_pos.z);
        ent.getAttrib<ecsRotation>()->setRotation(ai_quat.x, ai_quat.y, ai_quat.z, ai_quat.w);
        ent.getAttrib<ecsScale>()->setScale(ai_scale.x, ai_scale.y, ai_scale.z);
    }

    ent.getAttrib<ecsWorldTransform>();
    
    node_map[ent.getAttrib<ecsName>()->name] = ent.getId();

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
            std::shared_ptr<Mesh> mesh = assimpMergeMeshes(ai_meshes);
            
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
                            VFMT::ENUM_GENERIC::Position, VFMT::Position::count, 
                            VFMT::Position::gl_type, VFMT::Position::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->vao_cache->attach(seg.skin_data->normal_cache->getId(), 
                            VFMT::ENUM_GENERIC::Normal, VFMT::Normal::count, 
                            VFMT::Normal::gl_type, VFMT::Normal::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->vao_cache->attach(seg.skin_data->tangent_cache->getId(), 
                            VFMT::ENUM_GENERIC::Tangent, VFMT::Tangent::count, 
                            VFMT::Tangent::gl_type, VFMT::Tangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->vao_cache->attach(seg.skin_data->bitangent_cache->getId(), 
                            VFMT::ENUM_GENERIC::Bitangent, VFMT::Bitangent::count, 
                            VFMT::Bitangent::gl_type, VFMT::Bitangent::normalized ? GL_TRUE : GL_FALSE, 0, 0
                        );
                        seg.skin_data->pose_cache.reset(new gl::Buffer(GL_STATIC_DRAW, sizeof(float) * 16 * ai_mesh->mNumBones));

                        if(seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UV)) {
                            GLuint id = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UV)->getId();
                            seg.skin_data->vao_cache->attach(id, VFMT::ENUM_GENERIC::UV, 
                                VFMT::UV::count, VFMT::UV::gl_type, 
                                VFMT::UV::normalized ? GL_TRUE : GL_FALSE, 0, 0
                            );
                        }
                        if(seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UVLightmap)) {
                            GLuint id = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::UVLightmap)->getId();
                            seg.skin_data->vao_cache->attach(id, VFMT::ENUM_GENERIC::UVLightmap, 
                                VFMT::UVLightmap::count, VFMT::UVLightmap::gl_type, 
                                VFMT::UVLightmap::normalized ? GL_TRUE : GL_FALSE, 0, 0
                            );
                        }
                        if(seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::ColorRGBA)) {
                            GLuint id = seg.mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::ColorRGBA)->getId();
                            seg.skin_data->vao_cache->attach(id, VFMT::ENUM_GENERIC::ColorRGBA, 
                                VFMT::ColorRGBA::count, VFMT::ColorRGBA::gl_type, 
                                VFMT::ColorRGBA::normalized ? GL_TRUE : GL_FALSE, 0, 0
                            );
                        }
                        GLuint index_buf_id = seg.mesh->mesh.getIndexBuffer()->getId();
                        seg.skin_data->vao_cache->attachIndexBuffer(index_buf_id);

                        for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                            aiBone* ai_bone = ai_mesh->mBones[j];
                            std::string name(ai_bone->mName.data, ai_bone->mName.length);

                            auto it = node_map.find(name);
                            if(it != node_map.end()) {
                                seg.skin_data->bone_nodes.emplace_back(ecsEntityHandle(world, it->second));
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

bool assimpLoadFile(const char* filename, AssimpScene& scene) {
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

    scene.load(buffer.data(), buffer.size(), filename);
    return true;
}

entity_id assimpImportEcsScene(ecsWorld* world, const char* filename) {
    progressBegin("Fbx import", 100);

    progressStep(1.0f, "Reading fbx scene");
    AssimpScene assimp_scene;
    assimpLoadFile(filename, assimp_scene);
    progressStep(1.0f, "Constructing world");
    entity_id e = assimpImportEcsScene(world, &assimp_scene);
    
    progressEnd();
    return e;
}



void assimpImportAttachMeshes(
    const aiScene* ai_scene,
    Model_* model, 
    const std::unordered_map<const aiNode*, int>& node_map, 
    const std::unordered_map<std::string, int>& name_to_node_map, 
    const std::vector<const aiNode*>& mesh_nodes, 
    const std::vector<std::shared_ptr<Mesh>>& meshes
) {
    for(const auto ai_node : mesh_nodes) {
        for(int i = 0; i < ai_node->mNumMeshes; ++i) {
            auto mesh_id = ai_node->mMeshes[i];
            auto ai_mesh = ai_scene->mMeshes[mesh_id];

            ModelMesh mm;
            mm.node = node_map.find(ai_node)->second;
            mm.mesh = meshes[mesh_id];
            model->meshes.push_back(mm);

            if(ai_mesh->HasBones()) {
                std::vector<int>        bone_nodes;
                std::vector<gfxm::mat4> bind_transforms;
                for(unsigned j = 0; j < ai_mesh->mNumBones; ++j) {
                    aiBone* ai_bone = ai_mesh->mBones[j];
                    std::string name(ai_bone->mName.data, ai_bone->mName.length);

                    
                    auto it = name_to_node_map.find(name);
                    if(it != name_to_node_map.end()) {
                        bone_nodes.emplace_back(it->second);
                        bind_transforms.emplace_back(
                            gfxm::transpose(*(gfxm::mat4*)&ai_bone->mOffsetMatrix)
                        );
                    } else {
                        bone_nodes.emplace_back(-1);
                        bind_transforms.emplace_back(
                            gfxm::mat4(1.0f)
                        );
                    }
                }

                auto skin = new MeshDeformerSkin(
                    model,
                    &mm.mesh->mesh,
                    bone_nodes,
                    bind_transforms
                );
                model->meshes.back().deformer.reset(skin);
            }
        }
    }
}
void assimpImportModelNodeGraph(
    const aiScene* ai_scene, 
    const aiNode* ai_node, 
    Model_* model, 
    int node_id, 
    std::unordered_map<const aiNode*, int>& node_map, 
    std::unordered_map<std::string, int>& name_to_node_map,
    std::vector<const aiNode*>& mesh_nodes
) {
    auto node = model->getNode(node_id);
    node->name = ai_node->mName.C_Str();
    aiVector3D ai_pos;
    aiQuaternion ai_quat;
    aiVector3D ai_scale;
    ai_node->mTransformation.Decompose(ai_scale, ai_quat, ai_pos);
    node->translation = gfxm::vec3(ai_pos.x, ai_pos.y, ai_pos.z);
    node->rotation = gfxm::quat(ai_quat.x, ai_quat.y, ai_quat.z, ai_quat.w);
    node->scale = gfxm::vec3(ai_scale.x, ai_scale.y, ai_scale.z);

    node_map[ai_node] = node_id;
    name_to_node_map[ai_node->mName.C_Str()] = node_id;
    
    for(int i = 0; i < ai_node->mNumChildren; ++i) {
        int child_node_id = model->createNode(node_id);
        aiNode* ch = ai_node->mChildren[i];
        assimpImportModelNodeGraph(ai_scene, ch, model, child_node_id, node_map, name_to_node_map, mesh_nodes);
    }

    if(ai_node->mNumMeshes > 0) {
        mesh_nodes.push_back(ai_node);
    }
}
void      assimpImportEcsModel(Model_* model, AssimpScene* assimp_scene) {
    const aiScene* ai_scene = assimp_scene->getScene();

    std::vector<std::shared_ptr<Mesh>> meshes;
    for(int i = 0; i < ai_scene->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[i];
        std::vector<const aiMesh*> ai_meshes;
        ai_meshes.push_back(ai_mesh);
        std::shared_ptr<Mesh> mesh = assimpMergeMeshes(ai_meshes);
        meshes.push_back(mesh);     
    }

    std::unordered_map<const aiNode*, int> node_map;
    std::unordered_map<std::string, int> name_to_node_map;
    std::vector<const aiNode*> mesh_nodes;
    assimpImportModelNodeGraph(ai_scene, ai_scene->mRootNode, model, 0, node_map, name_to_node_map, mesh_nodes);
    assimpImportAttachMeshes(ai_scene, model, node_map, name_to_node_map, mesh_nodes, meshes);
    //assimpImportEcsModelGraph(ai_scene, model, 0, ai_scene->mRootNode, meshes);

    double scaleFactor = 1.0f;
    if(ai_scene->mMetaData) {
        if(ai_scene->mMetaData->Get("UnitScaleFactor", scaleFactor)) {
            if(scaleFactor == 0.0) scaleFactor = 1.0f;
            scaleFactor *= 0.01;
        }
    }
    model->getRootNode()->translation = gfxm::vec3(0,0,0);
    model->getRootNode()->rotation    = gfxm::quat(0,0,0,1);
    model->getRootNode()->scale = gfxm::vec3(scaleFactor,scaleFactor,scaleFactor);
}
bool      assimpImportEcsModel(Model_* model, const char* filename) {
    progressBegin("Fbx import", 100);
    progressStep(1.0f, "Reading fbx scene");
    AssimpScene assimp_scene;
    assimpLoadFile(filename, assimp_scene);
    progressStep(1.0f, "Constructing model");
    assimpImportEcsModel(model, &assimp_scene);
    progressEnd();
    return true;
}