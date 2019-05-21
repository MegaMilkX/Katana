#ifndef CREATE_IRRADIANCE_MAP_HPP
#define CREATE_IRRADIANCE_MAP_HPP

#include "../../resource/cube_map.hpp"
#include "../../shader_factory.hpp"

inline std::shared_ptr<CubeMap> createIrradianceMap(std::shared_ptr<CubeMap> skybox) {
    std::shared_ptr<CubeMap> cm_result;
    
    gl::ShaderProgram* prog = ShaderFactory::getOrCreate(
        "irradiance_gen",
        R"(#version 450
        in vec3 Position;
        out vec3 local_pos;
        uniform mat4 mat4_projection;
        uniform mat4 mat_view;
        void main() {
            local_pos = Position;
            gl_Position = mat_projection * mat_view * vec4(Position, 1.0);
        })",
        R"(#version 450
        out vec4 out_albedo;
        in vec3 local_pos;
        uniform samplerCube env_map;
        void main() {
            out_albedo = vec4(texture(env_map, local_pos).rgb, 1.0);
        })"
    );

    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo
    );

    cm_result->getId()
}

#endif
