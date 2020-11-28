#ifndef RENDER_MESH_DATA_HPP
#define RENDER_MESH_DATA_HPP

#include <stdint.h>
#include "vertex_format.hpp"
#include "../gl/vertex_buffer.hpp"
#include "../gl/indexed_mesh.hpp"


template<typename VFMT>
class MeshData {
    GLuint vao = 0;
    gl::VertexAttribBuffer attribs[VFMT::attrib_count];
    std::unique_ptr<gl::IndexBuffer> indices;
    int vertex_count = 0;
    int index_count = 0;

public:
    MeshData();
    ~MeshData();
    
    void upload         (int attrib, void* data, size_t size);
    void uploadIndices  (uint32_t* data, size_t count);

    void bind           (void);

    void draw           (GLenum prim);
    void drawIndexed    (GLenum prim);

    GLuint  getVao() const          { return vao; }
    int     getVertexCount() const  { return vertex_count; }
    int     getIndexCount() const   { return index_count; }
};

template<typename VFMT>
MeshData<VFMT>::MeshData() {
    glGenVertexArrays(1, &vao);
    
    auto vdesc = VFMT::getVertexDesc();
    glBindVertexArray(vao);
    for(int i = 0; i < VFMT::attrib_count; ++i) {
        attribs[i].bind();
        glEnableVertexAttribArray((GLuint)i);
        glVertexAttribPointer(
            (GLuint)i,
            vdesc->attribs[i].count,
            vdesc->attribs[i].gl_type,
            vdesc->attribs[i].normalized,
            0, 0
        );
    }
    glBindVertexArray(0);
}
template<typename VFMT>
MeshData<VFMT>::~MeshData() {
    glDeleteVertexArrays(1, &vao);
}

template<typename VFMT>
void MeshData<VFMT>::upload(int attrib, void* data, size_t size) {
    attribs[attrib].data(data, size);
    auto& attr_desc = VFMT::getAttribDesc(attrib);
    int vcount = size / (attr_desc.elem_size * attr_desc.count);
    vertex_count = std::max(vertex_count, vcount);
}
template<typename VFMT>
void MeshData<VFMT>::uploadIndices(uint32_t* data, size_t count) {
    if(!indices) {
        indices.reset(new gl::IndexBuffer());
        glBindVertexArray(vao);
        indices->bind();
        glBindVertexArray(0);
    }
    indices->data(data, count * sizeof(uint32_t));
    index_count = count;
}

template<typename VFMT>
void MeshData<VFMT>::bind(void) {
    glBindVertexArray(vao);
}

template<typename VFMT>
void MeshData<VFMT>::draw(GLenum prim) {
    assert(vertex_count);
    glDrawArrays(prim, 0, vertex_count);
}
template<typename VFMT>
void MeshData<VFMT>::drawIndexed(GLenum prim) {
    assert(vertex_count);
    assert(index_count);
    glDrawElements(prim, index_count, GL_UNSIGNED_INT, 0 /* offset */);
}


#endif
