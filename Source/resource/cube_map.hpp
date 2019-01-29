#ifndef CUBE_MAP_HPP
#define CUBE_MAP_HPP

#include "../gl/glextutil.h"

extern "C"{
#include "../lib/stb_image.h"
}

#include "resource.h"

#include "texture2d.h"
#include "../shader_factory.hpp"
#include "../gfxm.hpp"

inline void drawCube() {
    static const GLfloat g_vertex_buffer_data[] = {
        -1.0f,-1.0f,-1.0f, // triangle 1 : begin
        -1.0f,-1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f, // triangle 1 : end
        1.0f, 1.0f,-1.0f, // triangle 2 : begin
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f, // triangle 2 : end
        1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
        1.0f,-1.0f,-1.0f,
        1.0f, 1.0f,-1.0f,
        1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
        1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
        1.0f,-1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f,-1.0f,-1.0f,
        1.0f, 1.0f,-1.0f,
        1.0f,-1.0f,-1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f,-1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f,-1.0f, 1.0f
    };

    GLuint vao_handle = 0;
    GLuint vbuf;
    glGenBuffers(1, &vbuf);

    glGenVertexArrays(1, &vao_handle);
    glBindVertexArray(vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        0, 0
    );

    glBindBuffer(GL_ARRAY_BUFFER, vbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), (void*)g_vertex_buffer_data, GL_STREAM_DRAW);
    
    glDrawArrays(GL_TRIANGLES, 0, 12 * 3);

    glDeleteVertexArrays(1, &vao_handle);
    glDeleteBuffers(1, &vbuf);
}

class CubeMap : public Resource {
    RTTR_ENABLE(Resource)
public:
    CubeMap() {
        glGenTextures(1, &glId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    ~CubeMap() {
        glDeleteTextures(1, &glId);
    }

    void data(void* data, int width, int height, int bpp) {
        Texture2D tex;
        tex.Data((unsigned char*)data, width, height, bpp);
        GLuint tex_id = tex.GetGlName();

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

        GLuint capFbo, capRbo;
        glGenFramebuffers(1, &capFbo);
        glGenRenderbuffers(1, &capRbo);
        glBindFramebuffer(GL_FRAMEBUFFER, capFbo);
        glBindRenderbuffer(GL_RENDERBUFFER, capRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capRbo);

        glBindTexture(GL_TEXTURE_CUBE_MAP, glId);
        for(unsigned i = 0; i < 6; ++i) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, 0
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
        prog->use();
        glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (float*)&proj);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_id);

        glViewport(0,0,512,512);
        glBindFramebuffer(GL_FRAMEBUFFER, capFbo);
        for(unsigned i = 0; i < 6; ++i) {
            glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, glId, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            drawCube();
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /*
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, glId);

        GLenum format;
        if(bpp == 1) format = GL_RED;
        else if(bpp == 2) format = GL_RG;
        else if(bpp == 3) format = GL_RGB;
        else if(bpp == 4) format = GL_RGBA;
        else return;

        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, 
            0, GL_RGB16F, width, height, 
            0, format, GL_FLOAT, 
            data
        );
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 
            0, GL_RGB16F, width, height, 
            0, format, GL_FLOAT, 
            data
        );
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 
            0, GL_RGB16F, width, height, 
            0, format, GL_FLOAT, 
            data
        );
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 
            0, GL_RGB16F, width, height, 
            0, format, GL_FLOAT, 
            data
        );
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 
            0, GL_RGB16F, width, height, 
            0, format, GL_FLOAT, 
            data
        );
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 
            0, GL_RGB16F, width, height, 
            0, format, GL_FLOAT, 
            data
        );

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);*/
    }

    GLuint getId() {
        return glId;
    }

    virtual bool deserialize(std::istream& in, size_t sz) { 
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
        return true;
    }
private:
    GLuint glId;
};

#endif
