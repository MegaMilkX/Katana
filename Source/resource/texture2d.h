#ifndef TEXTURE2D_H
#define TEXTURE2D_H

#include "../gl/glextutil.h"
//#include <aurora/gfx.h>
//#include <util/load_asset.h>

extern "C"{
#include "../lib/stb_image.h"
}

#include "resource.h"

class Texture2D : public Resource
{
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
        _data = std::vector<unsigned char>(data, data + bpp * width * height);
        this->width = width;
        this->height = height;
        this->bpp = bpp;
        dirty = true;
    }
    
    GLuint GetGlName()
    {
        if(glTexName == 0)
            _initGlData();
        if(dirty)
            _reloadGlBuffer();
        return glTexName; 
    }

    int Width() { return width; }
    int Height() { return height; }

    virtual bool deserialize(std::istream& in, size_t sz) { 
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
        return true;
    }   
private:
    bool dirty;
    std::vector<unsigned char> _data;
    int width, height;
    int bpp;
    GLuint glTexName;
    
    void _initGlData()
    {
        glGenTextures(1, &glTexName);
        glBindTexture(GL_TEXTURE_2D, glTexName);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    void _reloadGlBuffer()
    {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        GLenum format;
        if(bpp == 1) format = GL_RED;
        else if(bpp == 2) format = GL_RG;
        else if(bpp == 3) format = GL_RGB;
        else if(bpp == 4) format = GL_RGBA;
        else return;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glTexName);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, (const GLvoid*)_data.data());
        dirty = false;
    }
};
STATIC_RUN(Texture2D)
{
    rttr::registration::class_<Texture2D>("Texture2D")
        .constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

#endif
