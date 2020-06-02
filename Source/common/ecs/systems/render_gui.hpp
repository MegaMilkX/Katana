#ifndef ECS_SYSTEM_RENDER_GUI_HPP
#define ECS_SYSTEM_RENDER_GUI_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

struct DrawQuadCmd2d {
    GLuint      texture;
    gfxm::mat4  transform;
    float       width;
    float       height;
    gfxm::vec4  color;
    gfxm::vec2  origin;
};

struct DrawList2d {
    const DrawQuadCmd2d*    quads;
    size_t                  quad_count;
};

class ecsTupleGuiImage : public ecsTuple<ecsWorldTransform, ecsGuiImage> {

};
class ecsTupleGuiElement : public ecsTuple<ecsGuiElement> {
public:
};

class ecsRenderGui : public ecsSystem<
    ecsTupleGuiImage,
    ecsTupleGuiElement
>{
    std::vector<DrawQuadCmd2d> quads;
public:
    DrawList2d getDrawList() {
        static float origin_offsets[] = {
            0.0f, 1.0f, 1.0f, 0.0f, 0.5f
        };

        quads.clear();
        for(auto& a : get_array<ecsTupleGuiImage>()) {
            auto q = a->get<ecsGuiImage>();
            auto wt = a->get<ecsWorldTransform>();
            gfxm::vec2 origin;
            origin.x = origin_offsets[q->align_h];
            origin.y = origin_offsets[q->align_v];
            quads.push_back(DrawQuadCmd2d{
                q->image ? q->image->GetGlName() : 0, // TODO: Default texture?
                wt->transform,
                q->width, q->height,
                q->color,
                origin
            });
        }

        for(auto& a : get_array<ecsTupleGuiElement>()) {
            auto e = a->get<ecsGuiElement>();
            gfxm::vec2 origin;
            origin.x = origin_offsets[e->align_h];
            origin.y = origin_offsets[e->align_v];
            quads.push_back(DrawQuadCmd2d{
                0, // TODO: Default texture?
                gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(e->position.x,e->position.y,0)),
                e->size.x, e->size.y,
                gfxm::vec4(1,1,1,1),
                origin
            });
        }

        return DrawList2d{
            quads.data(), quads.size()
        };
    }
};


#endif
