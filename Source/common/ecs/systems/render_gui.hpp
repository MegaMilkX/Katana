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

void draw2d(const DrawList2d& dl, float screenW, float screenH);

class ecsTupleGuiElement : public ecsTuple<ecsGuiElement> {};
class ecsTplGuiText : public ecsTuple<ecsGuiElement, ecsGuiText> {};
class ecsTplGuiImage : public ecsTuple<ecsGuiElement, ecsGuiImage> {};

class ecsRenderGui : public ecsSystem<
    ecsTupleGuiElement,
    ecsTplGuiText,
    ecsTplGuiImage
>{
    std::vector<DrawCmd2d> cmds;
    std::shared_ptr<Texture2D> texture_white_px;
    MeshData<VERTEX_FMT::LINE> lineMesh;

    void addQuad(const gfxm::rect& parent_rect, gfxm::rect& rect, ecsTupleGuiElement* elem, entity_id selected_ent) {
        auto e = elem->get<ecsGuiElement>();


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

        for(int i = get_dirty_index<ecsTplGuiText>(); i < count<ecsTplGuiText>(); ++i) {
            auto tuple = get<ecsTplGuiText>(i);
            auto e = tuple->get<ecsGuiElement>();
            auto t = tuple->get<ecsGuiText>();
            if(tuple->is_dirty<ecsGuiText>()) {
                float width = t->bb_width;
                float height = t->bb_height;
                e->bounding_rect = gfxm::rect(0, -height, width, 0);
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<ecsTplGuiText>();

        for(int i = get_dirty_index<ecsTplGuiImage>(); i < count<ecsTplGuiImage>(); ++i) {
            auto tuple = get<ecsTplGuiImage>(i);
            auto e = tuple->get<ecsGuiElement>();
            auto img = tuple->get<ecsGuiImage>();
            if(tuple->is_dirty<ecsGuiImage>() && img->image) {
                float width = img->image->Width();
                float height = img->image->Height();
                e->bounding_rect = gfxm::rect(0, -height, width, 0);
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<ecsTplGuiImage>();

        for(int i = get_dirty_index<ecsTupleGuiElement>(); i < count<ecsTupleGuiElement>(); ++i) {
            auto tuple = get<ecsTupleGuiElement>(i);
            auto e = tuple->get<ecsGuiElement>();

            gfxm::vec3 translation = gfxm::vec3(e->translation.x, e->translation.y, .0f);
            gfxm::quat rotation    = gfxm::angle_axis(gfxm::radian(e->rotation), gfxm::vec3(.0f, .0f, 1.0f));
            gfxm::vec3 scale       = gfxm::vec3(1, 1, 1);

            gfxm::mat4 local = gfxm::mat4(1.0f);
            local = gfxm::translate(local, translation);
            local = local * gfxm::to_mat4(rotation);
            local = gfxm::translate(
                local, 
                gfxm::vec3(-e->bounding_rect.max.x * e->origin.x, -e->bounding_rect.min.y * e->origin.y, 0)
            );
            local = gfxm::scale(local, scale);
            e->transform = local;

            if(tuple->get_parent()) {
                e->transform = tuple->get_parent()->get<ecsGuiElement>()->transform * e->transform;
                e->enabled = tuple->get_parent()->get<ecsGuiElement>()->isEnabled();
            }

            tuple->clear_dirty_signature();
        }
        clear_dirty<ecsTupleGuiElement>();

        
        for(auto& a : get_array<ecsTplGuiImage>()) {
            if(!a->get<ecsGuiElement>()->isEnabled()) {
                continue;
            }
            auto img = a->get<ecsGuiImage>();
            
            if(img->image) {
                gfxm::rect rect = a->get<ecsGuiElement>()->bounding_rect;

                DrawCmd2d cmd;
                memset(&cmd, 0, sizeof(cmd));
                cmd.type = DRAW_CMD_QUAD;
                cmd.clip_rect = gfxm::rect(0, 0, screenW, screenH);
                cmd.transform = a->get<ecsGuiElement>()->transform;
                cmd.quad.color = gfxm::vec4(1,1,1,1);
                cmd.quad.width = rect.max.x - rect.min.x;
                cmd.quad.height = rect.max.y - rect.min.y;
                cmd.quad.texture = img->image->GetGlName();
                cmds.push_back(cmd);
            }
        }

        for(auto& a : get_array<ecsTplGuiText>()) {
            if(!a->get<ecsGuiElement>()->isEnabled()) {
                continue;
            }
            auto text = a->get<ecsGuiText>();
            
            if(text->font && !text->str.empty()) {
                //text->rebuildBuffers();

                DrawCmd2d cmd;
                memset(&cmd, 0, sizeof(cmd));
                cmd.type = DRAW_CMD_TEXT;
                cmd.clip_rect = gfxm::rect(0, 0, screenW, screenH);
                cmd.transform = a->get<ecsGuiElement>()->transform;
                cmd.text.lookup_texture_width = text->font->getLookupTextureWidth(text->face_height);
                cmd.text.tex_atlas            = text->font->getAtlasTexture(text->face_height);
                cmd.text.tex_lookup           = text->font->getGlyphLookupTexture(text->face_height);
                cmd.text.vao                  = text->vao;
                cmd.text.vertex_count         = text->vertex_count;
                cmds.push_back(cmd);
            }
        }

        /*
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
        }*/

        return DrawList2d{
            cmds.data(), cmds.size()
        };
    }
};


#endif
