#include "cube_map.hpp"

CubeMap::CubeMap() {
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &glId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}
CubeMap::~CubeMap() {
    glDeleteTextures(1, &glId);
}

void CubeMap::data(void* data, int width, int height, int bpp) {
    const int side = 512;

    Texture2D tex;
    tex.Data((unsigned char*)data, width, height, bpp);
    GLuint tex_id = tex.GetGlName();
    //tex_id = retrieve<Texture2D>("images/test.png")->GetGlName();

    gl::ShaderProgram* prog = ShaderFactory::getOrCreate(
        "cube_map_gen",
        R"(#version 450
        in vec3 Position;
        out vec3 local_pos;
        uniform mat4 mat_projection;
        uniform mat4 mat_view;
        void main() {
            local_pos = Position;
            gl_Position = mat_projection * mat_view * vec4(Position, 1.0);
        })",
        R"(#version 450
        out vec4 out_albedo;
        in vec3 local_pos;
        uniform sampler2D tex_albedo;
        const vec2 invAtan = vec2(0.1591, 0.3183);
        vec2 sampleSphericalMap(vec3 v) {
            vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
            uv *= invAtan;
            uv += 0.5;
            return uv;
        }
        void main() {
            vec2 uv = sampleSphericalMap(normalize(local_pos));
            vec3 color = texture(tex_albedo, uv).rgb;
            out_albedo = vec4(color, 1.0);
        })"
    );

    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    for(unsigned i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB, side, side, 0, GL_RGB, GL_UNSIGNED_BYTE, 0
        );
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

    GLuint capFbo;
    glGenFramebuffers(1, &capFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, capFbo);

    GLuint capRbo;
    glGenRenderbuffers(1, &capRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, side, side);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capRbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, glId, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_WARN("Cube map fbo not complete!");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);


    gfxm::mat4 proj = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 10.0f);
    gfxm::mat4 views[] = 
    {
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3(-1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f,  1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f,  1.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f, -1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f, -1.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f,  0.0f,  1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f,  0.0f, -1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f))
    };

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    prog->use();
    glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_id);

    glViewport(0,0,side,side);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, capFbo);
    for(unsigned i = 0; i < 6; ++i) {
        glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&(views[i]));
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, glId, 0);

        drawCube();
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

/*
    glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
*/
    glDeleteFramebuffers(1, &capFbo);
    glDeleteRenderbuffers(1, &capRbo);
}

void CubeMap::makeIrradianceMap(std::shared_ptr<CubeMap> map) {
    const int side = 32;

    gl::ShaderProgram* prog = ShaderFactory::getOrCreate(
        "irradiance_gen",
        R"(#version 450
        in vec3 Position;
        out vec3 local_pos;
        uniform mat4 mat_projection;
        uniform mat4 mat_view;
        void main() {
            local_pos = Position;
            gl_Position = mat_projection * mat_view * vec4(Position, 1.0);
        })",
        R"(#version 450
        out vec4 out_albedo;
        in vec3 local_pos;
        uniform samplerCube env_map;
        const float PI = 3.14159265359;
        void main() {
            vec3 normal = normalize(local_pos);
            vec3 irr = vec3(0.0);
            vec3 up = vec3(0.0, 1.0, 0.0);
            vec3 right = cross(up, normal);
            up = cross(normal, right);
            float sampleDelta = 0.025;
            float nSamples = 0.0;
            for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
                for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
                    vec3 tgSample = vec3(
                        sin(theta) * cos(phi),
                        sin(theta) * sin(phi),
                        cos(theta)
                    );
                    vec3 sampleVec = 
                        tgSample.x * right + 
                        tgSample.y * up +
                        tgSample.z * normal;
                    irr += texture(env_map, sampleVec).rgb * cos(theta) * sin(theta);
                    nSamples++;
                }
            }
            irr = PI * irr * (1.0 / float(nSamples));

            out_albedo = vec4(irr, 1.0);
        })"
    );

    GLuint fbo, rbo;
    glGenFramebuffers(1, &fbo);
    glGenRenderbuffers(1, &rbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(
        GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, side, side
    );
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo
    );

    glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
    for(unsigned i = 0; i < 6; ++i) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            0, GL_RGB, side, side, 0, GL_RGB, GL_UNSIGNED_BYTE, 0
        );
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gfxm::mat4 proj = gfxm::perspective(gfxm::radian(90.0f), 1.0f, 0.1f, 10.0f);
    gfxm::mat4 views[] = 
    {
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3(-1.0f,  0.0f,  0.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f,  1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f,  1.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f, -1.0f,  0.0f), gfxm::vec3(0.0f,  0.0f, -1.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f,  0.0f,  1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f)),
        gfxm::lookAt(gfxm::vec3(0.0f, 0.0f, 0.0f), gfxm::vec3( 0.0f,  0.0f, -1.0f), gfxm::vec3(0.0f, -1.0f,  0.0f))
    };
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    prog->use();
    glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, map->getId());

    glViewport(0,0,side,side);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    for(int i = 0; i < 6; ++i) {
        glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, glId, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
        drawCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
}

GLuint CubeMap::getId() {
    return glId;
}

bool CubeMap::deserialize(in_stream& in, size_t sz) { 
    std::vector<char> buf;
    buf.resize(sz);
    in.read((char*)buf.data(), buf.size());

    stbi_set_flip_vertically_on_load(1);
    int w, h, bpp;
    unsigned char* data =
        stbi_load_from_memory((unsigned char*)buf.data(), sz, &w, &h, &bpp, 3);
    if(!data)
        return false;
    this->data(data, w, h, 3);
    stbi_image_free(data);
    return true;
}