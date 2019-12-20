#ifndef BLEND_TREE_2_HPP
#define BLEND_TREE_2_HPP

#include "resource.h"

#include "../gfxm.hpp"

struct BlendTree2Node {
    std::string name;
    gfxm::vec3 gui_color;
    gfxm::vec2 gui_pos;
    std::vector<size_t> input_ids;
    std::vector<size_t> output_ids;
};

class BlendTree2 : public Resource {
    RTTR_ENABLE(Resource)
public:
    std::vector<int>            data_points;
    std::vector<BlendTree2Node> nodes;

};
STATIC_RUN(BlendTree2) {
    rttr::registration::class_<BlendTree2>("BlendTree2")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
