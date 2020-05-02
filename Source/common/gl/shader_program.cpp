#include "shader_program.h"

#include <iostream>
#include <vector>

#include "../util/log.hpp"

#include "indexed_mesh.hpp"

#include "../render/shader_loader.hpp"

namespace gl {

ShaderProgram::ShaderProgram() {
    id = glCreateProgram();
    GL_LOG_ERROR("glCreateProgram");
}
ShaderProgram::~ShaderProgram() {
    glUseProgram(0);
    glDeleteProgram(id);
    GL_LOG_ERROR("glDeleteProgram");
}

void ShaderProgram::attachShader(Shader* shader)
{
    shader->attach(id);
}
void ShaderProgram::bindAttrib(GLuint loc, const std::string& name)
{
    glBindAttribLocation(id, loc, name.c_str());
    GL_LOG_ERROR("glBindAttribLocation");
}
void ShaderProgram::bindFragData(GLuint loc, const std::string& name)
{
    glBindFragDataLocation(id, loc, name.c_str());
    GL_LOG_ERROR("glBindFragDataLocation");
}
void ShaderProgram::setVertexFormat(const VERTEX_FMT::VERTEX_DESC* desc) {
    for(int i = 0; i < desc->attribCount; ++i) {
        bindAttrib(i, desc->attribs[i].name);
    }
}
void ShaderProgram::setTransformFeedbackFormat(const VERTEX_FMT::VERTEX_DESC* desc) {
    std::vector<const char*> names(desc->attribCount);
    for(int i = 0; i < names.size(); ++i) {
        names[i] = desc->attribs[i].out_name;
    }
    glTransformFeedbackVaryings(id, names.size(), names.data(), GL_SEPARATE_ATTRIBS);
    GL_LOG_ERROR("glTransformFeedbackVaryings");
}
bool ShaderProgram::link()
{
    glLinkProgram(id);
    GL_LOG_ERROR("glLinkProgram");

    GLint res = GL_TRUE;
    int infoLogLen = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &res); GL_LOG_ERROR("glGetShaderiv GL_LINK_STATUS");
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLen); GL_LOG_ERROR("glGetShaderiv GL_INFO_LOG_LENGTH");
    if (infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetProgramInfoLog(id, infoLogLen, NULL, &errMsg[0]);
        LOG("GLSL link: " << &errMsg[0]);
    }
    if (res == GL_FALSE)
        return false;

    loc_projection = getUniform("mat_projection");
    loc_view = getUniform("mat_view");
    loc_model = getUniform("mat_model");

    return true;
}
GLuint ShaderProgram::getUniform(const std::string& name)
{
    return glGetUniformLocation(id, name.c_str());
}
void ShaderProgram::use()
{
    glUseProgram(id);
    GL_LOG_ERROR("glUseProgram");  
}
bool ShaderProgram::validate() {
    int res;
    int infoLogLen;
    glValidateProgram(id);
    GL_LOG_ERROR("glValidateProgram");

    glGetProgramiv(id, GL_VALIDATE_STATUS, &res);
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLen);
    if(infoLogLen > 1)
    {
        std::vector<char> errMsg(infoLogLen + 1);
        glGetProgramInfoLog(id, infoLogLen, NULL, &errMsg[0]);
        LOG("GLSL Validate: " << &errMsg[0]);
    }
    if(res == GL_FALSE)
        return false;

    GLint count = 0;
    const GLsizei nameBufSize = 32;
    GLchar name[nameBufSize];
    GLsizei length;
    GLint size;
    GLenum type;
    glGetProgramiv(id, GL_ACTIVE_ATTRIBUTES, &count);
    LOG("Attribute count: " << count);
    for(int i = 0; i < count; ++i) {
        glGetActiveAttrib(id, (GLuint)i, nameBufSize, &length, &size, &type, name);
        LOG("Attrib " << i << ", type: " << type << ", name: " << name);
    }

    glGetProgramiv(id, GL_ACTIVE_UNIFORMS, &count);
    LOG("Uniform count: " << count);
    for(int i = 0; i < count; ++i) {
        glGetActiveUniform(id, (GLuint)i, nameBufSize, &length, &size, &type, name);
        GLint loc = glGetUniformLocation(id, name);
        uniforms.push_back(UniformInfo{ name, loc, type });
        LOG("Uniform " << i << ", type: " << type << ", name: " << name << ", retrieved id: " << loc);
    }


    return true;
}
GLuint ShaderProgram::getId() const
{
    return id;
}

void ShaderProgram::detachAll() {
    GLsizei count = 0;
    GLuint shaders[16];
    glGetAttachedShaders(id, 16, &count, shaders);
    for(GLsizei i = 0; i < count; ++i) {
        glDetachShader(id, shaders[i]);
    }
}


class glBuffer {
    GLuint id = 0;
    GLenum usage = 0;
public:
    glBuffer(GLenum usage, size_t size = 0)
    : usage(usage) {
        glGenBuffers(1, &id);
        if(size > 0) {
            glBindBuffer(GL_ARRAY_BUFFER, id);
            glBufferData(GL_ARRAY_BUFFER, size, 0, usage);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    ~glBuffer() {
        glDeleteBuffers(1, &id);
    }

    void upload(void* data, size_t size, GLenum new_usage) {
        glBindBuffer(GL_ARRAY_BUFFER, id);
        glBufferData(GL_ARRAY_BUFFER, size, data, usage);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void upload(void* data, size_t size) {
        upload(data, size, usage);
    }

    void bind(GLenum target) {
        glBindBuffer(target, id);
    }

    void bindBase(GLenum target, int index) {
        glBindBufferBase(target, index, id);
    }

};

struct DoubleVertexBuffer {
    GLuint vao;
    std::shared_ptr<glBuffer> pos_buffer;
    std::shared_ptr<glBuffer> col_buffer;
    std::shared_ptr<glBuffer> vel_buffer;
    gl::ShaderProgram* draw_prog;
    std::shared_ptr< gl::ShaderProgram> compute_prog;
};

static std::vector<gfxm::vec3> generateVertices(int count) {
  std::vector<gfxm::vec3> vertices(count);
  for (int i = 0; i < count; ++i) {
    vertices[i] = gfxm::vec3((rand() % 4000 - 2000) * 0.01f, (rand() % 4000 - 2000) * 0.01f, (rand() % 4000 - 2000) * 0.01f);
  }
  return vertices;
}

static std::vector<gfxm::vec4> generateVertices4(int count) {
  std::vector<gfxm::vec4> vertices(count);
  for (int i = 0; i < count; ++i) {
    vertices[i] = gfxm::vec4((rand() % 4000 - 2000) * 0.01f, (rand() % 4000 - 2000) * 0.01f, (rand() % 4000 - 2000) * 0.01f, 1.0f);
  }
  return vertices;
}

static std::vector<gfxm::vec4> generateColors4(int count) {
  std::vector<gfxm::vec4> vertices(count);
  for (int i = 0; i < count; ++i) {
    vertices[i] = gfxm::vec4(1,1,1,0);
  }
  return vertices;
}

static const int VERTEX_COUNT = 250000;
static DoubleVertexBuffer initBuffers() {
    DoubleVertexBuffer data = { 0 };

    auto vertices = generateVertices(VERTEX_COUNT);
    auto colors = generateColors4(VERTEX_COUNT);

    data.compute_prog.reset(new gl::ShaderProgram);
    gl::Shader compute(GL_COMPUTE_SHADER);
    compute.source(R"(
        #version 450
        layout(local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

        struct Pos {
            float x, y, z;
        };
        layout(std430, binding = 0) buffer Position {
            Pos Positions[];
        };
        layout(std430, binding = 1) buffer Color {
            vec4 Colors[];
        };
        layout(std430, binding = 2) buffer Velocity {
            vec4 Velocities[];
        };

        uniform float time;

        vec3 hsv2rgb(vec3 c)
        {
            vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
            return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }

        void main() {
            uint gid = gl_GlobalInvocationID.x;
            vec3 velo_add = Velocities[gid].xyz * 1.0/60.0 * 3.0;
            Positions[gid].x += velo_add.x;
            Positions[gid].y += velo_add.y;
            Positions[gid].z += velo_add.z;

            
            vec3 gravity_centers[3];
            gravity_centers[0] = vec3(0,0,cos(time * 0.1) * 50);
            gravity_centers[1] = vec3(cos(time) * 20, sin(time) * 20 + 20, 0);
            gravity_centers[2] = vec3(cos(time * 0.2) * 20, sin(time * 0.2) * 20 + 20, -20);
            

            for(int i = 1; i < 3; ++i) {
                vec3 dir = gravity_centers[i] - vec3(Positions[gid].x, Positions[gid].y, Positions[gid].z);
                float len = length(dir);
                float grav = (1000.0 - min(1000.0, len)) * 0.001;
                Velocities[gid].xyz += normalize(dir) * 9.8 * grav * 1.0/60.0 * 1.0;
            }

            Colors[gid] = vec4(hsv2rgb(vec3(length(Velocities[gid].xyz) * 0.005 + 0.3, 1, 1)), Colors[gid].a + 1.0/60.0 * 0.2);
        }
    )");
    if(!compute.compile()) {
        assert(false);
        return data;
    }
    data.compute_prog->attachShader(&compute);
    if(!data.compute_prog->link()) {
        assert(false);
        return data;
    }

    data.pos_buffer.reset(new glBuffer(GL_STATIC_DRAW));
    data.pos_buffer->upload(vertices.data(), sizeof(gfxm::vec3) * VERTEX_COUNT);
    data.col_buffer.reset(new glBuffer(GL_STATIC_DRAW));
    data.col_buffer->upload(colors.data(), sizeof(gfxm::vec4) * VERTEX_COUNT);
    data.vel_buffer.reset(new glBuffer(GL_STATIC_DRAW));
    data.vel_buffer->upload(0, sizeof(gfxm::vec4) * VERTEX_COUNT);

    data.draw_prog = shaderLoader().loadShaderProgram("shaders/test/point.glsl");
    data.draw_prog->bindAttrib(0, "Position");
    data.draw_prog->bindAttrib(1, "ColorRGBA");
    data.draw_prog->bindAttrib(2, "Velocity");
    data.draw_prog->bindFragData(0, "out_albedo");
    if(!data.draw_prog->link()) {
        assert(false);
    }

    return data;
}

void foo(const gfxm::mat4& view, const gfxm::mat4& proj) {
    static DoubleVertexBuffer dvb = initBuffers();

    dvb.pos_buffer->bindBase(GL_SHADER_STORAGE_BUFFER, 0);
    dvb.col_buffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);
    dvb.vel_buffer->bindBase(GL_SHADER_STORAGE_BUFFER, 2);
    dvb.compute_prog->use();
    static gfxm::vec3 grav_center;
    static float time = .0f;
    time += 1.0f/60.0f;
    glUniform1f(dvb.compute_prog->getUniform("time"), time);
    glDispatchCompute(VERTEX_COUNT / 512 + 1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    
    glBindVertexArray(vao);
    dvb.pos_buffer->bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray((GLuint)0);
    glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0/*stride*/, 0);
    dvb.col_buffer->bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray((GLuint)1);
    glVertexAttribPointer((GLuint)1, 4, GL_FLOAT, GL_FALSE, 0/*stride*/, 0);
    dvb.vel_buffer->bind(GL_ARRAY_BUFFER);
    glEnableVertexAttribArray((GLuint)2);
    glVertexAttribPointer((GLuint)2, 4, GL_FLOAT, GL_FALSE, 0/*stride*/, 0);

    glBlendFunc(GL_ONE, GL_ONE);

    dvb.draw_prog->use();
    glUniformMatrix4fv(dvb.draw_prog->getUniform("mat_view"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(dvb.draw_prog->getUniform("mat_proj"), 1, GL_FALSE, (float*)&proj);
    
    glDrawArrays(GL_POINTS, 0, VERTEX_COUNT);


    glDeleteVertexArrays(1, &vao);
    GL_LOG_ERROR("FOO");
}


}