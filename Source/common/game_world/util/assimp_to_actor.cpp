#include "assimp_to_actor.hpp"

#include "../../util/log.hpp"
#include "../../util/import/assimp.hpp"

static bool assimpLoadFile(const char* filename, AssimpScene& scene) {
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

static void assimpFinalizeMeshNodes(const aiScene* ai_scene, ktActor* actor, const std::vector<std::pair<const aiNode*, ktNodeMesh*>>& mesh_nodes, const std::vector<std::shared_ptr<Mesh>>& meshes) {
    for(auto pair : mesh_nodes) {
        auto ai_node = pair.first;
        auto kt_node = pair.second;

        // At this point ai_node->mNumMeshes is guaranteed to be at least 1
        kt_node->setMesh(meshes[ai_node->mMeshes[0]]);
        
        for(int i = 1; i < ai_node->mNumMeshes; ++i) {
            std::string node_name = MKSTR(kt_node->getName() << "_" << i);
            ktNodeMesh* new_node = new ktNodeMesh(actor);
            actor->addChild(kt_node, new_node);
            new_node->setName(node_name);
            new_node->setMesh(meshes[ai_node->mMeshes[i]]);
            
        }
    }
}

static void assimpImportGraph(const aiScene* ai_scene, const aiNode* ai_node, ktActor* actor, ktActorNode* node, std::vector<std::pair<const aiNode*, ktNodeMesh*>>& mesh_nodes) {
    node->setName(ai_node->mName.C_Str());
    aiVector3D ai_pos;
    aiQuaternion ai_quat;
    aiVector3D ai_scale;
    ai_node->mTransformation.Decompose(ai_scale, ai_quat, ai_pos);
    node->translation = gfxm::vec3(ai_pos.x, ai_pos.y, ai_pos.z);
    node->rotation = gfxm::quat(ai_quat.x, ai_quat.y, ai_quat.z, ai_quat.w);
    node->scale = gfxm::vec3(ai_scale.x, ai_scale.y, ai_scale.z);

    for(int i = 0; i < ai_node->mNumChildren; ++i) {
        ktActorNode* child = 0;
        aiNode* ai_child = ai_node->mChildren[i];
        if(ai_child->mNumMeshes > 0) {
            child = new ktNodeMesh(actor);
            mesh_nodes.push_back(std::pair<const aiNode*, ktNodeMesh*>(ai_child, (ktNodeMesh*)child));
        } else {
            child = new ktActorNode(actor);
        }
        assimpImportGraph(ai_scene, ai_child, actor, child, mesh_nodes);

        actor->addChild(node, child);
    }
}

static void assimpImportAsActor(ktActor* actor, AssimpScene* assimp_scene) {
    const aiScene* ai_scene = assimp_scene->getScene();

    std::vector<std::shared_ptr<Mesh>> meshes;
    for(int i = 0; i < ai_scene->mNumMeshes; ++i) {
        aiMesh* ai_mesh = ai_scene->mMeshes[i];
        std::vector<const aiMesh*> ai_meshes;
        ai_meshes.push_back(ai_mesh);
        std::shared_ptr<Mesh> mesh = assimpMergeMeshes(ai_meshes);
        meshes.push_back(mesh);     
    }

    std::vector<std::pair<const aiNode*, ktNodeMesh*>> mesh_nodes;
    assimpImportGraph(ai_scene, ai_scene->mRootNode, actor, actor->getRoot(), mesh_nodes);
    
    // Root transform needs to be fixed
    double scaleFactor = assimp_scene->fbxScaleFactor;
    actor->getRoot()->translation = gfxm::vec3(0,0,0);
    actor->getRoot()->rotation    = gfxm::quat(0,0,0,1);
    actor->getRoot()->scale = gfxm::vec3(scaleFactor, scaleFactor, scaleFactor);
    
    assimpFinalizeMeshNodes(ai_scene, actor, mesh_nodes, meshes);
}

bool assimpImportAsActor(ktActor* actor, const char* filename) {
    AssimpScene scene;
    assimpLoadFile(filename, scene);
    assimpImportAsActor(actor, &scene);
    return true;
}