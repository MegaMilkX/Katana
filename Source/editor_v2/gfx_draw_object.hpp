#ifndef GFX_DRAW_OBJECT_HPP
#define GFX_DRAW_OBJECT_HPP

#include "transform.hpp"
#include "../common/gl/indexed_mesh.hpp"
#include "../common/shader_factory.hpp"

struct GfxDrawObject {
    gl::IndexedMesh* mesh = 0;
    TransformNode* transform = 0;
};

#endif
