#ifndef GL_INDEXED_MESH_HPP
#define GL_INDEXED_MESH_HPP

#include "glextutil.h"
#include "error.hpp"
#include "vertex_buffer.hpp"

#include <memory>
#include <map>

#include "../util/data_stream.hpp"

#include "../render/vertex_format.hpp"

namespace gl {
/*
enum ATTRIB_INDEX {
    VERTEX_ATTRIB_FIRST = 0,
    POSITION = VERTEX_ATTRIB_FIRST,
    UV,
    UV_LIGHTMAP,
    NORMAL,
    TANGENT,
    BITANGENT,
    BONE_INDEX4,
    BONE_WEIGHT4,
    COLOR_RGBA,
    VERTEX_ATTRIB_COUNT
};

struct AttribDesc {
    std::string name;
    GLint size;
    GLenum type;
    GLboolean normalized;
};

inline const size_t getAttribElemTypeSize(GLenum type) {
    size_t sz = 0;
    switch(type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        sz = 1;
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        sz = 2;
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
        sz = 4;
        break;
    case GL_HALF_FLOAT:
        sz = 2;
        break;
    case GL_FLOAT:
        sz = 4;
        break;
    case GL_DOUBLE:
        sz = 8;
        break;
    case GL_FIXED:
    case GL_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_2_10_10_10_REV:
    case GL_UNSIGNED_INT_10F_11F_11F_REV:
    default:
        assert(false);
        break;
    }
    return sz;
}

inline const AttribDesc& getAttribDesc(ATTRIB_INDEX index) {
    static AttribDesc table[] = {
        { "Position", 3, GL_FLOAT, GL_FALSE },
        { "UV", 2, GL_FLOAT, GL_FALSE },
        { "UVLightmap", 2, GL_FLOAT, GL_FALSE },
        { "Normal", 3, GL_FLOAT, GL_FALSE },
        { "Tangent", 3, GL_FLOAT, GL_FALSE },
        { "Bitangent", 3, GL_FLOAT, GL_FALSE },
        { "BoneIndex4", 4, GL_FLOAT, GL_FALSE },
        { "BoneWeight4", 4, GL_FLOAT, GL_FALSE },
        { "ColorRGBA", 4, GL_UNSIGNED_BYTE, GL_TRUE }
    };
    return table[index];
}*/

class IndexedMesh {
public:
    IndexedMesh() {
        glGenVertexArrays(1, &id);
    }
    ~IndexedMesh() {
        glDeleteVertexArrays(1, &id);
    }
    void setAttribData(int index, void* data, size_t size) {
        auto& inf = VERTEX_FMT::GENERIC::getAttribDesc(index);
        setAttribData(index, data, size, inf.count, inf.gl_type, inf.normalized ? GL_TRUE : GL_FALSE);
    }
    void setAttribData(int index, void* data, size_t size, GLint attrib_size, GLenum type, GLboolean normalized) {
        assert(size);
        attribs[index].reset(new gl::VertexAttribBuffer());
        attribs[index]->data(data, size);
        glBindVertexArray(id);
        glEnableVertexAttribArray((GLuint)index);
        //glVertexAttribPointer((GLuint)index, getAttribDesc(index).size, getAttribDesc(index).type, getAttribDesc(index).normalized, 0/*stride*/, 0);
        glVertexAttribPointer((GLuint)index, attrib_size, type, normalized, 0/*stride*/, 0);
        glBindVertexArray(0);
    }
    void setIndices(const uint32_t* data, size_t count) {
        indices.data((uint32_t*)data, sizeof(uint32_t) * count);
        index_count = count;
        glBindVertexArray(id);
        indices.bind();
    }

    gl::VertexAttribBuffer* getAttribBuffer(int index) {
        auto it = attribs.find(index);
        if(it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }
    size_t getAttribDataSize(int index) {
        if(attribs.count(index)) {
            return attribs[index]->getDataSize();
        }
        return 0;
    }
    bool copyAttribData(int index, void* dest) {
        if(attribs.count(index)) {
            return attribs[index]->copyData(dest, attribs[index]->getDataSize());
        }
        return 0;
    }
    size_t getIndexDataSize() {
        return indices.getDataSize();
    }
    bool copyIndexData(void* dest) {
        return indices.copyData(dest, indices.getDataSize());
    }

    GLuint getVao() const {
        return id;
    }
    size_t getIndexCount() const {
        return index_count;
    }
    void bind() {
        glBindVertexArray(id);
    }

    void serialize(out_stream& out) {
        out.write<uint8_t>(attribs.size());

        for(auto& kv : attribs) {
            auto ptr = kv.second;
            
            uint8_t attrib_id = (uint8_t)kv.first;
            out.write((char*)&attrib_id, sizeof(attrib_id));
            uint64_t data_size = (uint64_t)kv.second->getDataSize();
            out.write((char*)&data_size, sizeof(data_size));

            std::vector<char> buf;
            buf.resize((size_t)data_size);
            ptr->copyData((void*)buf.data(), (size_t)data_size);
            out.write(buf.data(), (size_t)data_size);
        }

        uint64_t index_data_size = indices.getDataSize();
        out.write((char*)&index_data_size, sizeof(index_data_size));

        std::vector<char> buf;
        buf.resize((size_t)index_data_size);
        indices.copyData((void*)buf.data(), (size_t)index_data_size);
        out.write(buf.data(), buf.size());
    }
    void deserialize(in_stream& in) {
        uint8_t attrib_count = 0;
        in.read((char*)&attrib_count, sizeof(attrib_count));
        for(uint8_t i = 0; i < attrib_count; ++i) {
            uint8_t attrib_id_ui8 = 0;
            in.read((char*)&attrib_id_ui8, sizeof(attrib_id_ui8));
            uint64_t data_size = 0;
            in.read((char*)&data_size, sizeof(data_size));
            if(data_size == 0) {
                continue;
            }

            std::vector<char> buf;
            buf.resize(data_size);
            in.read((char*)buf.data(), buf.size());

            auto& desc = VERTEX_FMT::GENERIC::getAttribDesc(attrib_id_ui8);
            setAttribData(
                (int)attrib_id_ui8, 
                buf.data(), buf.size(),
                desc.count,
                desc.gl_type,
                desc.normalized ? GL_TRUE : GL_FALSE
            );
        }

        uint64_t index_data_size = 0;
        in.read((char*)&index_data_size, sizeof(index_data_size));
        std::vector<char> buf;
        buf.resize(index_data_size);
        in.read((char*)buf.data(), buf.size());
        setIndices((uint32_t*)buf.data(), buf.size() / sizeof(uint32_t));
    }
private:
    GLuint id;
    std::map<int, std::shared_ptr<gl::VertexAttribBuffer>> attribs;
    gl::IndexBuffer indices;
    size_t index_count = 0;
};

}

#endif
