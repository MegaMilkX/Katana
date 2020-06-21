#ifndef ECS_SYSTEM_RENDER_GUI_HPP
#define ECS_SYSTEM_RENDER_GUI_HPP


#include "../system.hpp"
#include "../attribs/base_attribs.hpp"
#include "../../render/mesh_data.hpp"

enum DRAW_CMD_2D {
    DRAW_CMD_QUAD,
    DRAW_CMD_TEXT,
    DRAW_CMD_LINE
};

struct DrawCmd2d {
    DrawCmd2d() {
        memset(this, 0, sizeof(DrawCmd2d));
    }

    DRAW_CMD_2D     type;
    gfxm::mat4      transform;
    gfxm::rect      clip_rect;

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
        struct {
            GLuint          vao;
            int             vertex_count;
            gfxm::vec4      color;
        } line;
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
    MeshData<VERTEX_FMT::LINE> lineMesh;

    void addQuad(const gfxm::rect& parent_rect, gfxm::rect& rect, ecsTupleGuiElement* elem, entity_id selected_ent) {
        auto e = elem->get<ecsGuiElement>();

        auto layoutFloat = e->getLayoutFloating();
        auto layoutDock = e->getLayoutDocking();
        bool isLayoutAuto = e->isLayout(GUI_ELEM_LAYOUT_AUTO);
        
        gfxm::vec2 pos(rect.min.x, rect.min.y);
        gfxm::vec2 size = rect.max;
        if(layoutFloat) {
            pos = pos + layoutFloat->position;
            size = layoutFloat->size;
        } if (layoutDock) {
            auto side = layoutDock->side;
            float dock_sz = layoutDock->size;
            if(side == GUI_ELEM_DOCK_TOP) {
                size.y = dock_sz;
                rect.min.y -= dock_sz;
                rect.max.y -= dock_sz;
            } else if(side == GUI_ELEM_DOCK_BOTTOM) {
                pos.y -= size.y - dock_sz;
                size.y = dock_sz;
                rect.max.y -= dock_sz;
            } else if(side == GUI_ELEM_DOCK_LEFT) {
                size.x = dock_sz;
                rect.min.x += dock_sz;
                rect.max.x -= dock_sz;
            } else if(side == GUI_ELEM_DOCK_RIGHT) {
                pos.x += size.x - dock_sz;
                size.x = dock_sz;
                rect.max.x -= dock_sz;
            }
        }
        pos.x += e->margin.x;
        pos.y -= e->margin.y;
        size.x -= e->margin.x + e->margin.z;
        size.y -= e->margin.y + e->margin.w;

        pos.x = ceil(pos.x);
        pos.y = ceil(pos.y);
        gfxm::rect rect_fin(
            pos, 
            size
        );
        gfxm::rect rect_inner(
            pos,
            size
        );
        rect_inner.min.x += e->padding.x;
        rect_inner.min.y -= e->padding.y;
        rect_inner.max.x -= e->padding.x + e->padding.z;
        rect_inner.max.y -= e->padding.y + e->padding.w;

        e->rendered_pos = rect_inner.min;
        e->rendered_size = rect_inner.max;

        GLuint tex = texture_white_px->GetGlName();
        if(e->bg_image) {
            tex = e->bg_image->GetGlName();
        }

        if(e->is<GuiElemText>()) {
            auto text = e->getParams<GuiElemText>();
            if (!text->text.empty() && text->font) {
                text->textBuffer.rebuild(text->font.get(), text->text, text->face_height, text->alignment, gfxm::vec2(rect_fin.max.x, rect_fin.max.y));

                if(text->vertical_center) {
                    pos.y -= size.y * 0.5f - text->textBuffer.bb_size.y * 0.5f;
                }

                DrawCmd2d cmd;
                memset(&cmd, 0, sizeof(cmd));
                cmd.type = DRAW_CMD_TEXT;
                cmd.transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(pos.x, pos.y, 0));
                cmd.clip_rect = parent_rect;
                cmd.text.vao = text->textBuffer.vao;
                cmd.text.tex_atlas = text->font->getAtlasTexture(text->face_height);
                cmd.text.tex_lookup = text->font->getGlyphLookupTexture(text->face_height);
                cmd.text.lookup_texture_width = text->font->getLookupTextureWidth(text->face_height);
                cmd.text.vertex_count = text->textBuffer.vertex_count;
                cmds.push_back(cmd);
            }
        } else if(e->is<GuiElemImage>()) {
            auto img = e->getParams<GuiElemImage>();

            GLuint tex = texture_white_px->GetGlName();
            if(img->image) {
                tex = img->image->GetGlName();
            }
            
            DrawCmd2d cmd;
            memset(&cmd, 0, sizeof(cmd));
            cmd.type = DRAW_CMD_QUAD;
            cmd.transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(rect_fin.min.x, rect_fin.min.y, 0));
            cmd.clip_rect = parent_rect;
            cmd.quad.width = rect_fin.max.x;
            cmd.quad.height = rect_fin.max.y;
            cmd.quad.color = gfxm::vec4(1,1,1,1);
            cmd.quad.texture = tex;
            cmds.push_back(cmd);
        } else if(e->is<GuiElemContainer>()) {
            auto cont = e->getParams<GuiElemContainer>();

            DrawCmd2d cmd;
            memset(&cmd, 0, sizeof(cmd));
            cmd.type = DRAW_CMD_QUAD;
            cmd.transform = gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(rect_fin.min.x, rect_fin.min.y, 0));
            cmd.clip_rect = parent_rect;
            cmd.quad.width = rect_fin.max.x;
            cmd.quad.height = rect_fin.max.y;
            cmd.quad.color = e->bg_color;
            cmd.quad.texture = tex;
            cmds.push_back(cmd);

            auto child = (ecsTupleGuiElement*)elem->first_child;
            while(child) {
                addQuad(rect_fin, rect_inner, child, selected_ent);
                child = (ecsTupleGuiElement*)child->next_sibling;
            }
        }
    }

public:
    ecsRenderGui() {
        texture_white_px.reset(new Texture2D());
        unsigned char wht[4] = { 255, 255, 255, 255 };
        texture_white_px->Data(wht, 1, 1, 4);
    }

    ecsGuiElement* hitTest(float screenW, float screenH, float x, float y) {
        return 0;
    }

    DrawList2d makeDrawList(float screenW, float screenH, entity_id selected_ent) {
        cmds.clear();

        std::vector<ecsTupleGuiElement*> root_elements;
        for(auto& a : get_array<ecsTupleGuiElement>()) {
            auto e = a->get<ecsGuiElement>();
            if(a->parent == 0) {
                root_elements.push_back(a);
            }
        }

        gfxm::rect rect(0, 0, screenW, screenH);
        for(auto& a : root_elements) {
            addQuad(gfxm::rect(0, 0, screenW, screenH), rect, a, selected_ent);           
        }

        return DrawList2d{
            cmds.data(), cmds.size()
        };
    }
};


#endif
