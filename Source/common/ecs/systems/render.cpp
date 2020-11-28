#include "render.hpp"

#include "../renderer.hpp"


void computeSkinCache(ecsMeshes::Segment* seg, const gfxm::mat4& root_m4) {
    static std::unique_ptr<gl::ShaderProgram> p = createSkinComputeShader();

    glBindVertexArray(0);

    std::vector<gfxm::mat4> bone_transforms(seg->skin_data->bone_nodes.size());
    for(int i = 0; i < seg->skin_data->bone_nodes.size(); ++i) {
        auto t = seg->skin_data->bone_nodes[i];
        if(t) {
            // HINT: bind_transforms are already inverted it seems
            bone_transforms[i] = root_m4 * t.findAttrib<ecsWorldTransform>()->getTransform() * seg->skin_data->bind_transforms[i];
        } else {
            bone_transforms[i] = root_m4 * gfxm::mat4(1.0f);
        }
    }
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    seg->skin_data->pose_cache->upload(bone_transforms.data(), bone_transforms.size() * sizeof(gfxm::mat4));

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    GLuint pos_id = seg->mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::Position)->getId();
    GLuint nrm_id = seg->mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::Normal)->getId();
    GLuint tan_id = seg->mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::Tangent)->getId();
    GLuint bitan_id = seg->mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::Bitangent)->getId();
    GLuint bi4_id = seg->mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::BoneIndex4)->getId();
    GLuint bw4_id = seg->mesh->mesh.getAttribBuffer(VFMT::ENUM_GENERIC::BoneWeight4)->getId();
    

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pos_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nrm_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tan_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bitan_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bi4_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bw4_id);
    seg->skin_data->pose_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 6);
    seg->skin_data->position_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 7);
    seg->skin_data->normal_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 8);
    seg->skin_data->tangent_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 9);
    seg->skin_data->bitangent_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 10);

    p->use();

    glDispatchCompute(seg->mesh->vertexCount() / 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 10, 0);

    // Done!
}