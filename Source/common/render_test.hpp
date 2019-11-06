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

struct BoundUniform {
    GLint loc;
    size_t offset;
};

template<typename STRUCT>
class UniformBinding {
    std::vector<BoundUniform> vec_int;
    std::vector<BoundUniform> vec_int2;
    std::vector<BoundUniform> vec_int3;
    std::vector<BoundUniform> vec_int4;
    std::vector<BoundUniform> vec_float;
    std::vector<BoundUniform> vec_float2;
    std::vector<BoundUniform> vec_float3;
    std::vector<BoundUniform> vec_float4;
    std::vector<BoundUniform> vec_mat3;
    std::vector<BoundUniform> vec_mat4;

    std::map<std::pair<std::string, GLenum>, size_t> offsets;
    void set(const char* name, GLenum type, size_t offset) {
        offsets[std::make_pair(std::string(name), type)] = offset;
    }
public:
    void enable(const char* name, GLenum type, GLint location) {
        auto it = offsets.find(std::make_pair(std::string(name), type));
        if(it == offsets.end()) {
            return;
        }
        std::vector<BoundUniform>* p_vec;
        switch(type) {
        case GL_INT: p_vec = &vec_int; break;
        case GL_INT_VEC2: p_vec = &vec_int2; break;
        case GL_INT_VEC3: p_vec = &vec_int3; break;
        case GL_INT_VEC4: p_vec = &vec_int4; break;
        case GL_FLOAT: p_vec = &vec_float; break;
        case GL_FLOAT_VEC2: p_vec = &vec_float2; break;
        case GL_FLOAT_VEC3: p_vec = &vec_float3; break;
        case GL_FLOAT_VEC4: p_vec = &vec_float4; break;
        case GL_FLOAT_MAT3: p_vec = &vec_mat3; break;
        case GL_FLOAT_MAT4: p_vec = &vec_mat4; break;
        }
        if(!p_vec) {
            LOG_WARN("UniformBinding: tried to enable uniform of unsupported type: " << type);
            return;
        }
        p_vec->emplace_back(BoundUniform{ location, it->second });
    }

    void set(const char* name, int STRUCT::*member) {
        set(name, GL_INT, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::ivec2 STRUCT::*member) {
        set(name, GL_INT_VEC2, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::ivec3 STRUCT::*member) {
        set(name, GL_INT_VEC3, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::ivec4 STRUCT::*member) {
        set(name, GL_INT_VEC4, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, float STRUCT::*member) {
        set(name, GL_FLOAT, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::vec2 STRUCT::*member) {
        set(name, GL_FLOAT_VEC2, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::vec3 STRUCT::*member) {
        set(name, GL_FLOAT_VEC3, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::vec4 STRUCT::*member) {
        set(name, GL_FLOAT_VEC4_ARB, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::mat3 STRUCT::*member) {
        set(name, GL_FLOAT_MAT3, (size_t)&(((STRUCT*)0)->*member));
    }
    void set(const char* name, gfxm::mat4 STRUCT::*member) {
        set(name, GL_FLOAT_MAT4, (size_t)&(((STRUCT*)0)->*member));
    }

    void upload(const STRUCT& data) {
        for(size_t i = 0; i < vec_int.size(); ++i) {
            auto& u = vec_int[i];
            GLint* ptr = (GLint*)((char*)&data + u.offset);
            glUniform1i(u.loc, *ptr);
        }
        for(size_t i = 0; i < vec_int2.size(); ++i) {
            auto& u = vec_int2[i];
            GLint* ptr = (GLint*)((char*)&data + u.offset);
            glUniform2i(u.loc, *(ptr), *(ptr + 1));
        }
        for(size_t i = 0; i < vec_int3.size(); ++i) {
            auto& u = vec_int3[i];
            GLint* ptr = (GLint*)((char*)&data + u.offset);
            glUniform3i(u.loc, *(ptr), *(ptr + 1), *(ptr + 2));
        }
        for(size_t i = 0; i < vec_int4.size(); ++i) {
            auto& u = vec_int4[i];
            GLint* ptr = (GLint*)((char*)&data + u.offset);
            glUniform4i(u.loc, *(ptr), *(ptr + 1), *(ptr + 2), *(ptr + 3));
        }
        for(size_t i = 0; i < vec_float.size(); ++i) {
            auto& u = vec_float[i];
            GLfloat* ptr = (GLfloat*)((char*)&data + u.offset);
            glUniform1f(u.loc, *(ptr));
        }
        for(size_t i = 0; i < vec_float2.size(); ++i) {
            auto& u = vec_float2[i];
            GLfloat* ptr = (GLfloat*)((char*)&data + u.offset);
            glUniform2f(u.loc, *(ptr), *(ptr + 1));
        }
        for(size_t i = 0; i < vec_float3.size(); ++i) {
            auto& u = vec_float3[i];
            GLfloat* ptr = (GLfloat*)((char*)&data + u.offset);
            glUniform3f(u.loc, *(ptr), *(ptr + 1), *(ptr + 2));
        }
        for(size_t i = 0; i < vec_float4.size(); ++i) {
            auto& u = vec_float4[i];
            GLfloat* ptr = (GLfloat*)((char*)&data + u.offset);
            glUniform4f(u.loc, *(ptr), *(ptr + 1), *(ptr + 2), *(ptr + 3));
        }
        for(size_t i = 0; i < vec_mat3.size(); ++i) {
            auto& u = vec_mat3[i];
            GLfloat* ptr = (GLfloat*)((char*)&data + u.offset);
            glUniformMatrix3fv(u.loc, 1, GL_FALSE, ptr);
        }
        for(size_t i = 0; i < vec_mat4.size(); ++i) {
            auto& u = vec_mat4[i];
            GLfloat* ptr = (GLfloat*)((char*)&data + u.offset);
            glUniformMatrix4fv(u.loc, 1, GL_FALSE, ptr);
        }
    }
};

template<typename UNIFORMS, typename INSTANCE_UNIFORMS>
struct DrawBucket {
    gl::ShaderProgram* prog;
    UniformBinding<UNIFORMS> bindingGlobal;
    UniformBinding<INSTANCE_UNIFORMS> bindingLocal;
    UNIFORMS uniform_buffer;
    struct Instance {
        GLuint vao;
        int indexCount;
        int indexOffset;
        INSTANCE_UNIFORMS uniforms;
    };
    std::vector<Instance> instances;    

    template<typename T>
    void enable(T UNIFORMS::*ptr, const char* name) {
        bindingGlobal.set(name, ptr);
    }
    template<typename T>
    void enable(T INSTANCE_UNIFORMS::*ptr, const char* name) {
        bindingLocal.set(name, ptr);
    }

    void setShader(gl::ShaderProgram* prog) {
        this->prog = prog;
        for(size_t i = 0; i < prog->uniformCount(); ++i) {
            auto& inf = prog->getUniformInfo(i);
            bindingGlobal.enable(inf.name.c_str(), inf.type, inf.loc);
            bindingLocal.enable(inf.name.c_str(), inf.type, inf.loc);
        }
    }

    Instance& addDrawCmd() {
        instances.emplace_back(Instance{});
        return instances.back();
    }

    void draw() {
        prog->use();
        bindingGlobal.upload(uniform_buffer);

        for(size_t i = 0; i < instances.size(); ++i) {
            bindingLocal.upload(instances[i].uniforms);

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


#endif
