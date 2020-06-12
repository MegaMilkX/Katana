#ifndef ECS_SYSTEM_RENDER_GUI_HPP
#define ECS_SYSTEM_RENDER_GUI_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"

enum DRAW_CMD_2D {
    DRAW_CMD_QUAD,
    DRAW_CMD_TEXT
};

struct DrawCmd2d {
    DrawCmd2d() {
        memset(this, 0, sizeof(DrawCmd2d));
    }

    DRAW_CMD_2D     type;
    gfxm::mat4      transform;
    gfxm::vec4      clip_rect;

    union {
        struct {
            GLuint      texture;
            float       width;
            float       height;
            gfxm::vec4  color;
        } quad;
        struct {
            GLuint      tex_atlas;
            GLuint      tex_lookup;
            GLuint      vao;
            uint32_t    vertex_count;
            float       lookup_texture_width;
        } text;
    };
};

struct DrawList2d {
    const DrawCmd2d*        array;
    size_t                  count;
};

class ecsTupleGuiElement : public ecsTuple<ecsGuiElement, ecsOptional<ecsGuiImage>, ecsOptional<ecsGuiText>> {
public:
};

class ecsRenderGui : public ecsSystem<
    ecsTupleGuiElement
>{
    std::vector<DrawCmd2d> cmds;
    std::shared_ptr<Texture2D> texture_white_px;

public:
    ecsRenderGui() {
        texture_white_px.reset(new Texture2D());
        unsigned char wht[4] = { 255, 255, 255, 255 };
        texture_white_px->Data(wht, 1, 1, 4);
    }

    DrawList2d makeDrawList(float screenW, float screenH) {
        cmds.clear();

        std::vector<ecsTupleGuiElement*> root_elements;
        for(auto& a : get_array<ecsTupleGuiElement>()) {
            auto e = a->get<ecsGuiElement>();
            if(a->parent == 0) {
                root_elements.push_back(a);
            }
        }

        std::function<void(ecsTupleGuiElement*, const gfxm::vec2&, const gfxm::vec4&)> add_quad_fn;
        add_quad_fn = [this, &add_quad_fn](ecsTupleGuiElement* elem, const gfxm::vec2& pos,  const gfxm::vec4& clip_rect){
            auto e = elem->get<ecsGuiElement>();
            auto img = elem->get_optional<ecsGuiImage>();
            auto text = elem->get_optional<ecsGuiText>();
            GLuint tex = (img && img->image) ? img->image->GetGlName() : texture_white_px->GetGlName();
            gfxm::vec4 color = (img) ? img->color : gfxm::vec4(0,0,0,1);
            
            DrawCmd2d cmd;
            memset(&cmd, 0, sizeof(cmd));
            if(text && text->font) {
                cmd.type = DRAW_CMD_TEXT;
                cmd.transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x,pos.y,0));
                cmd.clip_rect = clip_rect;
                cmd.text.vao = text->vao;
                cmd.text.tex_atlas = text->font->getAtlasTexture(text->face_height);
                cmd.text.tex_lookup = text->font->getGlyphLookupTexture(text->face_height);
                cmd.text.lookup_texture_width = text->font->getLookupTextureWidth(text->face_height);
                cmd.text.vertex_count = text->vertex_count;
            } else {
                cmd.type = DRAW_CMD_QUAD;
                cmd.transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x,pos.y,0));
                cmd.clip_rect = clip_rect;
                cmd.quad.width = e->size.x;
                cmd.quad.height = e->size.y;
                cmd.quad.color = color;
                cmd.quad.texture = tex;
            }
            cmds.push_back(cmd);

            gfxm::vec2 child_pos_base;
            if(e->align_h == GUI_ALIGN_LEFT) {
                child_pos_base.x = pos.x;
            } else if (e->align_h == GUI_ALIGN_CENTER) {
                child_pos_base.x = pos.x + (int)(e->size.x * 0.5f);
            } else if (e->align_h == GUI_ALIGN_RIGHT) {
                child_pos_base.x = pos.x + e->size.x;
            }
            if(e->align_v == GUI_ALIGN_TOP) {
                child_pos_base.y = pos.y + e->size.y;
            } else if(e->align_v == GUI_ALIGN_CENTER) {
                child_pos_base.y = pos.y + (int)(e->size.y * 0.5f);
            } else if(e->align_v == GUI_ALIGN_BOTTOM) {
                child_pos_base.y = pos.y;
            }
            gfxm::vec2 child_pos = child_pos_base;
            gfxm::vec2 size = e->size;

            static const float origin_offsets[] = {
                0.0f,
                1.0f,
                1.0f,
                0.0f,
                0.5f
            };

            gfxm::vec4 this_clip_rect = gfxm::vec4(pos.x,pos.y,e->size.x,e->size.y);
            auto child = (ecsTupleGuiElement*)elem->first_child;
            float line_offset = .0f;
            while(child) {
                float child_width = .0f;
                float child_height = .0f;
                auto child_text = child->get_optional<ecsGuiText>();
                auto child_elem = child->get<ecsGuiElement>();
                if(child_text) {
                    child_width = child_text->bb_width;
                    child_height = child_text->bb_height;
                } else {
                    child_width = child_elem->size.x;
                    child_height = child_elem->size.y;
                }

                float available_line_len = .0f;
                float advance_len = .0f;
                if(e->child_layout == GUI_CHILD_LAYOUT_HORI) {
                    advance_len = child_width;
                    if(e->align_h == GUI_ALIGN_LEFT) {
                        available_line_len = pos.x + e->size.x - child_pos.x;
                    } else if(e->align_h == GUI_ALIGN_RIGHT) {
                        available_line_len = child_pos.x - pos.x;
                    }
                } else if(e->child_layout == GUI_CHILD_LAYOUT_VERT) {
                    advance_len = child_height;
                    if(e->align_h == GUI_ALIGN_TOP) {
                        available_line_len = pos.y - child_pos.y;
                    } else if(e->align_h == GUI_ALIGN_BOTTOM) {
                        available_line_len = child_pos.y - pos.y + e->size.y;
                    }
                }
                
                if(available_line_len < advance_len) {
                    if(e->child_layout == GUI_CHILD_LAYOUT_HORI) {
                        child_pos.x = child_pos_base.x;
                        if(e->align_v == GUI_ALIGN_TOP) {
                            child_pos.y -= line_offset;
                        } else {
                            child_pos.y += line_offset;
                        }
                    } else if(e->child_layout == GUI_CHILD_LAYOUT_VERT) {
                        child_pos.y = child_pos_base.y;
                        if(e->align_h == GUI_ALIGN_RIGHT) {
                            child_pos.x -= line_offset;
                        } else {
                            child_pos.x += line_offset;
                        }
                    }
                    line_offset = .0f;
                }

                gfxm::vec2 quad_pos = child_pos;
                quad_pos.x -= child_width * origin_offsets[e->align_h];
                quad_pos.y -= child_height * origin_offsets[e->align_v];

                add_quad_fn(child, quad_pos, this_clip_rect);

                if(e->child_layout == GUI_CHILD_LAYOUT_HORI) {
                    line_offset = gfxm::_max(line_offset, child_height);
                    if(e->align_h == GUI_ALIGN_LEFT) {
                        child_pos.x += child_width;
                    } else if(e->align_h == GUI_ALIGN_RIGHT) {
                        child_pos.x -= child_width;
                    }
                } else if(e->child_layout == GUI_CHILD_LAYOUT_VERT) {
                    line_offset = gfxm::_max(line_offset, child_width);
                    if(e->align_v == GUI_ALIGN_TOP) {
                        child_pos.y -= child_height;
                    } else if(e->align_v == GUI_ALIGN_BOTTOM) {
                        child_pos.y += child_height;
                    }
                }

                child = (ecsTupleGuiElement*)child->next_sibling;
            }
        };
        for(auto& a : root_elements) {
            add_quad_fn(a, a->get<ecsGuiElement>()->position, gfxm::vec4(0,0,screenW,screenH));
            
        }

        return DrawList2d{
            cmds.data(), cmds.size()
        };
    }
};


#endif
