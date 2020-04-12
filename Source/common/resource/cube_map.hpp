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

#include "../draw_primitive.hpp"

#include "resource_tree.hpp"

class CubeMap : public Resource {
    RTTR_ENABLE(Resource)
public:
    CubeMap();
    ~CubeMap();

    void resize(int width, int height);
    void data(void* data, int width, int height, int bpp);

    void makeIrradianceMap(std::shared_ptr<CubeMap> map);

    GLuint getId();

    virtual bool deserialize(in_stream& in, size_t sz);
private:
    GLuint glId;
};

#endif
