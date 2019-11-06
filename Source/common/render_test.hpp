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

    void upload(gl::ShaderProgram* prog) {
        glUniformMatrix4fv(prog->getUniform("mat_model"), 1, GL_FALSE, (GLfloat*)&model);
        glUniform3f(prog->getUniform("color"), color.x, color.y, color.z);
    }
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


class BoundShader {
    gl::ShaderProgram* prog = 0;
public:
    void init(gl::ShaderProgram* p) {
        
    }
};


#endif
