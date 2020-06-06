#include "render.hpp"


static std::unique_ptr<gl::ShaderProgram> createComputeShader() {
    std::unique_ptr<gl::ShaderProgram> p(new gl::ShaderProgram);

    gl::Shader sh(GL_COMPUTE_SHADER);
    sh.source(R"(
        #version 450
        layout(local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

        struct VEC3 {
            float x, y, z;
        };
        layout(std430, binding = 0) buffer Position {
            VEC3 Positions[];
        };
        layout(std430, binding = 1) buffer Normal {
            VEC3 Normals[];
        };
        layout(std430, binding = 2) buffer Tangent {
            VEC3 Tangents[];
        };
        layout(std430, binding = 3) buffer Bitangent {
            VEC3 Bitangents[];
        };
        layout(std430, binding = 4) buffer BoneIndex4Buf {
            vec4 BoneIndex4[];
        };
        layout(std430, binding = 5) buffer BoneWeight4Buf {
            vec4 BoneWeight4[];
        };
        layout(std430, binding = 6) buffer Pose {
            mat4 Poses[];
        };

        layout(std430, binding = 7) buffer out_Position {
            VEC3 out_Positions[];
        };
        layout(std430, binding = 8) buffer out_Normal {
            VEC3 out_Normals[];
        };
        layout(std430, binding = 9) buffer out_Tangent {
            VEC3 out_Tangents[];
        };
        layout(std430, binding = 10) buffer out_Bitangent {
            VEC3 out_Bitangents[];
        };

        
        void main() {
            uint gid = gl_GlobalInvocationID.x;

            vec3 P = vec3(Positions[gid].x, Positions[gid].y, Positions[gid].z);
            vec3 N = vec3(Normals[gid].x, Normals[gid].y, Normals[gid].z);
            vec3 T = vec3(Tangents[gid].x, Tangents[gid].y, Tangents[gid].z);
            vec3 B = vec3(Bitangents[gid].x, Bitangents[gid].y, Bitangents[gid].z);

            ivec4 bi = ivec4(
                int(BoneIndex4[gid].x), int(BoneIndex4[gid].y),
                int(BoneIndex4[gid].z), int(BoneIndex4[gid].w)
            );
            vec4 w = BoneWeight4[gid];/*
            if(w.x + w.y + w.z + w.w > 1.0) {
                w = normalize(w);
            }*/

            mat4 m0 = Poses[bi.x];
            mat4 m1 = Poses[bi.y];
            mat4 m2 = Poses[bi.z];
            mat4 m3 = Poses[bi.w];

            vec3 NS = (
                m0 * vec4(N, 0.0) * w.x +
                m1 * vec4(N, 0.0) * w.y +
                m2 * vec4(N, 0.0) * w.z +
                m3 * vec4(N, 0.0) * w.w
            ).xyz;
            vec3 TS = (
                m0 * vec4(T, 0.0) * w.x +
                m1 * vec4(T, 0.0) * w.y +
                m2 * vec4(T, 0.0) * w.z +
                m3 * vec4(T, 0.0) * w.w
            ).xyz;
            vec3 BS = (
                m0 * vec4(B, 0.0) * w.x +
                m1 * vec4(B, 0.0) * w.y +
                m2 * vec4(B, 0.0) * w.z +
                m3 * vec4(B, 0.0) * w.w
            ).xyz;

            NS = normalize(NS);
            TS = normalize(TS);
            BS = normalize(BS);

            vec4 PS = (
                m0 * vec4(P, 1.0) * w.x +
                m1 * vec4(P, 1.0) * w.y +
                m2 * vec4(P, 1.0) * w.z +
                m3 * vec4(P, 1.0) * w.w
            );

            out_Positions[gid].x = PS.x;
            out_Positions[gid].y = PS.y;
            out_Positions[gid].z = PS.z;
            out_Normals[gid].x = NS.x;
            out_Normals[gid].y = NS.y;
            out_Normals[gid].z = NS.z;
            out_Tangents[gid].x = TS.x;
            out_Tangents[gid].y = TS.y;
            out_Tangents[gid].z = TS.z;
            out_Bitangents[gid].x = BS.x;
            out_Bitangents[gid].y = BS.y;
            out_Bitangents[gid].z = BS.z;
        }
    )");
    assert(sh.compile());
    p->attachShader(&sh);
    assert(p->link());



    return p;
}


void computeSkinCache(ecsMeshes::Segment* seg, const gfxm::mat4& root_m4) {
    static std::unique_ptr<gl::ShaderProgram> p = createComputeShader();

    std::vector<gfxm::mat4> bone_transforms(seg->skin_data->bone_nodes.size());
    for(int i = 0; i < seg->skin_data->bone_nodes.size(); ++i) {
        auto t = seg->skin_data->bone_nodes[i];
        if(t) {
            // HINT: bind_transforms are already inverted it seems
            bone_transforms[i] = root_m4 * t->getTransform() * seg->skin_data->bind_transforms[i];
        } else {
            bone_transforms[i] = root_m4 * gfxm::mat4(1.0f);
        }
    }
    seg->skin_data->pose_cache->upload(bone_transforms.data(), bone_transforms.size() * sizeof(gfxm::mat4));

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    GLuint pos_id = seg->mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Position)->getId();
    GLuint nrm_id = seg->mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Normal)->getId();
    GLuint tan_id = seg->mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Tangent)->getId();
    GLuint bitan_id = seg->mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::Bitangent)->getId();
    GLuint bi4_id = seg->mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::BoneIndex4)->getId();
    GLuint bw4_id = seg->mesh->mesh.getAttribBuffer(VERTEX_FMT::ENUM_GENERIC::BoneWeight4)->getId();
    GLuint pose_id = seg->skin_data->pose_cache->getId();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, pos_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, nrm_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, tan_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, bitan_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, bi4_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, bw4_id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, pose_id);
    seg->skin_data->position_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 7);
    seg->skin_data->normal_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 8);
    seg->skin_data->tangent_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 9);
    seg->skin_data->bitangent_cache->bindBase(GL_SHADER_STORAGE_BUFFER, 10);

    p->use();
    glDispatchCompute(seg->mesh->vertexCount() / 512 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    // Done!
}