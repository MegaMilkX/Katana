#ifndef GFX_COMMON_HPP
#define GFX_COMMON_HPP

namespace gfx {
namespace gl {

const int MAX_BONE_COUNT = 256;

enum UNIFORM_BUFFER_BINDING_INDEX {
    UBO_COMMON3D = 0,
    UBO_BONES3D,
    UBO_LIGHT3D
};

}
}

#endif
