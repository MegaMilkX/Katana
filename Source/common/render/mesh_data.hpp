#ifndef RENDER_MESH_DATA_HPP
#define RENDER_MESH_DATA_HPP

#include <stdint.h>
#include "vertex_format.hpp"
#include "../gl/vertex_buffer.hpp"
#include "../gl/indexed_mesh.hpp"


template<typename VERTEX_FMT>
class MeshData {
    GLuint vao = 0;
    gl::VertexAttribBuffer attribs[VERTEX_FMT::attrib_count];
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

template<typename VERTEX_FMT>
MeshData<VERTEX_FMT>::MeshData() {
    glGenVertexArrays(1, &vao);
    
    auto vdesc = VERTEX_FMT::getVertexDesc();
    glBindVertexArray(vao);
    for(int i = 0; i < VERTEX_FMT::attrib_count; ++i) {
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
template<typename VERTEX_FMT>
MeshData<VERTEX_FMT>::~MeshData() {
    glDeleteVertexArrays(1, &vao);
}

template<typename VERTEX_FMT>
void MeshData<VERTEX_FMT>::upload(int attrib, void* data, size_t size) {
    attribs[attrib].data(data, size);
    auto& attr_desc = VERTEX_FMT::getAttribDesc(attrib);
    int vcount = size / (attr_desc.elem_size * attr_desc.count);
    vertex_count = std::max(vertex_count, vcount);
}
template<typename VERTEX_FMT>
void MeshData<VERTEX_FMT>::uploadIndices(uint32_t* data, size_t count) {
    if(!indices) {
        indices.reset(new gl::IndexBuffer());
        glBindVertexArray(vao);
        indices->bind();
        glBindVertexArray(0);
    }
    indices->data(data, count * sizeof(uint32_t));
    index_count = count;
}

template<typename VERTEX_FMT>
void MeshData<VERTEX_FMT>::bind(void) {
    glBindVertexArray(vao);
}

template<typename VERTEX_FMT>
void MeshData<VERTEX_FMT>::draw(GLenum prim) {
    assert(vertex_count);
    glDrawArrays(prim, 0, vertex_count);
}
template<typename VERTEX_FMT>
void MeshData<VERTEX_FMT>::drawIndexed(GLenum prim) {
    assert(vertex_count);
    assert(index_count);
    glDrawElements(prim, index_count, GL_UNSIGNED_INT, 0 /* offset */);
}


#endif
