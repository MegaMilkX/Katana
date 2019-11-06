#ifndef RENDER_TEST_HPP
#define RENDER_TEST_HPP

#include <memory.h>
#include <assert.h>
#include "gl/glextutil.h"
#include "gl/shader_program.h"
#include "gl/indexed_mesh.hpp"

#include <map>

enum SHADER_PARAM {
    SP_FLOAT3,
    SP_MAT4,
    SHADER_PARAM_MAX
};


struct UniformsGeneric {
    float time;
    gfxm::mat4 view;
    gfxm::mat4 projection;
    
    void upload(gl::ShaderProgram* prog) {
        glUniform1f(prog->getUniform("time"), time);
        glUniformMatrix4fv(prog->getUniform("mat_view"), 1, GL_FALSE, (GLfloat*)&view);
        glUniformMatrix4fv(prog->getUniform("mat_projection"), 1, GL_FALSE, (GLfloat*)&projection);
    }
};
struct UniformsBasicMesh {
    gfxm::mat4 model;
    gfxm::vec3 color;
    gfxm::mat4 bones[128];

    void upload(gl::ShaderProgram* prog) {
        glUniformMatrix4fv(prog->getUniform("mat_model"), 1, GL_FALSE, (GLfloat*)&model);
        glUniform3f(prog->getUniform("color"), color.x, color.y, color.z);
    }
};

enum UPLOAD_TYPE {
   UPLOAD_FLOAT,
   UPLOAD_FLOAT2,
   UPLOAD_FLOAT3,
   UPLOAD_FLOAT4,
   UPLOAD_MAT2,
   UPLOAD_MAT3,
   UPLOAD_MAT4,
   UPLOAD_TYPE_COUNT
};

template<typename UNIFORMS, typename INSTANCE_UNIFORMS>
struct DrawBucket {
    gl::ShaderProgram* prog;
    UNIFORMS uniform_buffer;
    struct Instance {
        GLuint vao;
        int indexCount;
        int indexOffset;
    };
    std::vector<INSTANCE_UNIFORMS> instance_uniform_buffer;
    std::vector<Instance> instances;

    struct BoundUniform {
        GLint loc;
        size_t offset;
    };
    BoundUniform bound_uniforms[UPLOAD_TYPE_COUNT];
    

    template<typename T>
    void enable(T UNIFORMS::*ptr, const char* name) {
        size_t offset = (size_t)&(((UNIFORMS*)0)->*ptr);
        size_t size = sizeof(T);
        LOG("offset: " << offset << ", size: " << size);
    }
    template<typename T>
    void enable(T INSTANCE_UNIFORMS::*ptr, const char* name) {
        size_t offset = (size_t)&(((INSTANCE_UNIFORMS*)0)->*ptr);
        size_t size = sizeof(T);
        LOG("offset: " << offset << ", size: " << size);
    }

    void setShader(gl::ShaderProgram* prog) {
        this->prog = prog;
        for(size_t i = 0; i < prog->uniformCount(); ++i) {
            auto& inf = prog->getUniformInfo(i);
            
        }
    }

    void draw() {
        prog->use();
        uniform_buffer.upload(prog);

        for(size_t i = 0; i < instances.size(); ++i) {
            instance_uniform_buffer[i].upload(prog);

            glBindVertexArray(instances[i].vao);
            glDrawElements(
                GL_TRIANGLES, 
                instances[i].indexCount, 
                GL_UNSIGNED_INT, 
                (GLvoid*) instances[i].indexOffset
            );
        }
    }
};


inline void foo_render() {
    DrawBucket<UniformsGeneric, UniformsBasicMesh> bucket;
    bucket.enable(&UniformsGeneric::projection, "mat_projection");
    bucket.enable(&UniformsGeneric::view, "mat_view");
    bucket.enable(&UniformsGeneric::time, "fTime");
    bucket.enable(&UniformsBasicMesh::model, "mat_model");
    bucket.enable(&UniformsBasicMesh::color, "color");
}


class BoundShader {
    gl::ShaderProgram* prog = 0;
public:
    void init(gl::ShaderProgram* p) {
        
    }
};


#endif
