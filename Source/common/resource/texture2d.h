#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "../gl/glextutil.h"
#include "../gl/error.hpp"
//#include <aurora/gfx.h>
//#include <util/load_asset.h>

extern "C"{
#include "../lib/stb_image.h"
}
#include "../lib/stb_image_write.h"

#include "resource.h"

inline void stbi_write_cb(void *context, void *data, int size) {
    out_stream* out = (out_stream*)context;
    out->write(data, size);
}

class Texture2D : public Resource {
    RTTR_ENABLE(Resource)
public:
    Texture2D()
    : glTexName(0), dirty(true)
    {       
    }
    
    ~Texture2D()
    {
        glDeleteTextures(1, &glTexName);
    }
    
    void Data(unsigned char* data, int width, int height, int bpp)
    {
        elem_type = GL_UNSIGNED_BYTE;
        _data = std::vector<unsigned char>(data, data + bpp * width * height);
        this->width = width;
        this->height = height;
        this->bpp = bpp;
        dirty = true;
    }

    void Data(float* data, int width, int height, int bpp)
    {
        elem_type = GL_FLOAT;
        _data = std::vector<unsigned char>((unsigned char*)data, (unsigned char*)(data + bpp * width * height));
        this->width = width;
        this->height = height;
        this->bpp = bpp;
        dirty = true;
    }

    void Filter(GLint filter) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, GetGlName());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    GLuint GetGlName()
    {
        if(glTexName == 0)
            _initGlData();
        if(dirty)
            _reloadGlBuffer();
        return glTexName; 
    }

    unsigned char* getData() { return _data.data(); }
    int getBpp() { return bpp; }
    int Width() { return width; }
    int Height() { return height; }

    void serialize(out_stream& out) {
        stbi_flip_vertically_on_write(1);
        stbi_write_tga_to_func(&stbi_write_cb, &out, Width(), Height(), getBpp(), getData());
    }
    virtual bool deserialize(in_stream& in, size_t sz) { 
        std::vector<char> buf;
        buf.resize(sz);
        in.read((char*)buf.data(), buf.size());

        stbi_set_flip_vertically_on_load(1);
        int w, h, bpp;
        unsigned char* data =
            stbi_load_from_memory((unsigned char*)buf.data(), sz, &w, &h, &bpp, 4);
        if(!data)
            return false;
        Data(data, w, h, 4);
        stbi_image_free(data);
        return true;
    }

    virtual const char* getWriteExtension() const { return "tga"; }
private:
    bool dirty;
    std::vector<unsigned char> _data;
    int width, height;
    int bpp;
    GLuint glTexName;
    GLenum elem_type = GL_UNSIGNED_BYTE;
    
    void _initGlData()
    {
        glGenTextures(1, &glTexName);
        GL_LOG_ERROR("glGenTextures");
        glActiveTexture(GL_TEXTURE0);
        GL_LOG_ERROR("glActiveTexture");
        glBindTexture(GL_TEXTURE_2D, glTexName);
        GL_LOG_ERROR("glBindTexture");

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        GL_LOG_ERROR("glTexParameteri GL_TEXTURE_MIN_FILTER");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_LOG_ERROR("glTexParameteri GL_TEXTURE_MAG_FILTER");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GL_LOG_ERROR("glTexParameteri GL_TEXTURE_WRAP_S");
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        GL_LOG_ERROR("glTexParameteri GL_TEXTURE_WRAP_T");

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void _reloadGlBuffer()
    {
        GLenum format;
        if(bpp == 1) format = GL_RED;
        else if(bpp == 2) format = GL_RG;
        else if(bpp == 3) format = GL_RGB;
        else if(bpp == 4) format = GL_RGBA;
        else return;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glTexName);
        GL_LOG_ERROR("glBindTexture");
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GL_LOG_ERROR("glPixelStorei");
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, elem_type, (const GLvoid*)_data.data());
        GL_LOG_ERROR("glTexImage2D");
        glGenerateMipmap(GL_TEXTURE_2D);
        GL_LOG_ERROR("glGenerateMipmap");
        dirty = false;
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};
STATIC_RUN(Texture2D)
{
    rttr::registration::class_<Texture2D>("Texture2D")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

#endif
