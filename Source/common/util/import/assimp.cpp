#include "assimp.hpp"


std::shared_ptr<Mesh> assimpMergeMeshes(const std::vector<const aiMesh*>& ai_meshes) {
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

    mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::Position, vertices.data(), vertices.size() * sizeof(float));
    if(normal_layers.size() > 0) {
        mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::Normal, normal_layers[0].data(), normal_layers[0].size() * sizeof(float));
    }
    if(uv_layers.size() > 0) {
        mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::UV, uv_layers[0].data(), uv_layers[0].size() * sizeof(float));
    }
    if(rgb_layers.size() > 0) {
        mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::ColorRGBA, rgb_layers[0].data(), rgb_layers[0].size() * sizeof(uint8_t));
    }
    if(!boneIndices.empty() && !boneWeights.empty()) {
        mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::BoneIndex4, boneIndices.data(), boneIndices.size() * sizeof(gfxm::vec4));
        mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::BoneWeight4, boneWeights.data(), boneWeights.size() * sizeof(gfxm::vec4));
    }
    mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::Tangent, tangent.data(), tangent.size() * sizeof(float));
    mesh->mesh.setAttribData(VFMT::ENUM_GENERIC::Bitangent, bitangent.data(), bitangent.size() * sizeof(float));

    mesh->mesh.setIndices(indices.data(), indices.size());

    mesh->makeAabb();

    return mesh;
}