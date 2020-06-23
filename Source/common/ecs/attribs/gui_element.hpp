#ifndef ECS_GUI_ELEMENT_HPP
#define ECS_GUI_ELEMENT_HPP

#include "../attribute.hpp"
#include "../common/util/imgui_helpers.hpp"

#include "gui/gui_elem_params.hpp"

enum GUI_ELEM_LAYOUT {
    GUI_ELEM_LAYOUT_FIRST,

    GUI_ELEM_LAYOUT_AUTO = GUI_ELEM_LAYOUT_FIRST,
    GUI_ELEM_LAYOUT_FLOAT,
    GUI_ELEM_LAYOUT_DOCK,

    GUI_ELEM_LAYOUT_COUNT
};
inline const char* getGuiElemLayoutName(GUI_ELEM_LAYOUT layout) {
    const char* name = "UNKNOWN";
    switch(layout) {
    case GUI_ELEM_LAYOUT_AUTO: name = "Auto"; break;
    case GUI_ELEM_LAYOUT_FLOAT: name = "Float"; break;
    case GUI_ELEM_LAYOUT_DOCK: name = "Dock"; break;
    default: assert(false);
    };
    return name;
}

enum GUI_ELEM_DOCK {
    GUI_ELEM_DOCK_FIRST,

    GUI_ELEM_DOCK_TOP = GUI_ELEM_DOCK_FIRST,
    GUI_ELEM_DOCK_BOTTOM,
    GUI_ELEM_DOCK_LEFT,
    GUI_ELEM_DOCK_RIGHT,

    GUI_ELEM_DOCK_COUNT
};
inline const char* getGuiElemDockName(GUI_ELEM_DOCK dock) {
    const char* name = "UNKNOWN";
    switch(dock) {
    case GUI_ELEM_DOCK_TOP: name = "Top"; break;
    case GUI_ELEM_DOCK_BOTTOM: name = "Bottom"; break;
    case GUI_ELEM_DOCK_LEFT: name = "Left"; break;
    case GUI_ELEM_DOCK_RIGHT: name = "Right"; break;
    default: assert(false);
    };
    return name;
}

struct GuiLayoutBase {
    virtual ~GuiLayoutBase() {}
};
struct GuiLayoutFloating : public GuiLayoutBase {
    gfxm::vec2 position = gfxm::vec2(0,0);
    gfxm::vec2 size     = gfxm::vec2(100, 100);
};
struct GuiLayoutDocked : public GuiLayoutBase {
    GUI_ELEM_DOCK side = GUI_ELEM_DOCK_TOP;
    float         size = 100.0f;
};

enum GUI_ALIGNMENT {
    GUI_ALIGN_LEFT      = 0,
    GUI_ALIGN_RIGHT     = 1,
    GUI_ALIGN_TOP       = 2,
    GUI_ALIGN_BOTTOM    = 3,
    GUI_ALIGN_CENTER    = 4
};
enum GUI_CHILD_LAYOUT {
    GUI_CHILD_LAYOUT_HORI,
    GUI_CHILD_LAYOUT_VERT
};
#include "gui/gui_attrib.hpp"
class ecsGuiElement : public ecsAttrib<ecsGuiElement> {
friend ecsRenderGui;
    bool                enabled             = true;
    int                 order_index         = 0;
    gfxm::vec2          translation;
    float               rotation;
    gfxm::vec2          origin              = gfxm::vec2(0.5f, 0.5f);

    gfxm::mat4          transform = gfxm::mat4(1.0f);
    gfxm::rect          bounding_rect;

public:
    void setEnabled(bool e) { enabled = e; getEntityHdl().signalUpdate<ecsGuiElement>(); }
    void setOrderIndex(int idx) { order_index = idx; getEntityHdl().signalUpdate<ecsGuiElement>(); }
    void setTranslation(float x, float y) { translation = gfxm::vec2(x, y); getEntityHdl().signalUpdate<ecsGuiElement>(); }
    void setRotation(float degrees) { rotation = degrees; getEntityHdl().signalUpdate<ecsGuiElement>(); }
    void setOrigin(float x, float y) { setOrigin(gfxm::vec2(x, y)); }
    void setOrigin(const gfxm::vec2& origin) { this->origin = origin; getEntityHdl().signalUpdate<ecsGuiElement>(); }

    void translate(float x, float y) { translate(gfxm::vec2(x, y)); }
    void translate(const gfxm::vec2& offs) { translation += offs; getEntityHdl().signalUpdate<ecsGuiElement>(); }

    bool              isEnabled() const { return enabled; }
    const float       getRotation() const { return rotation; }
    const gfxm::vec2& getOrigin() const { return origin; }
    const gfxm::mat4& getTransform() const { return transform; }

    const gfxm::rect& getBoundingRect() const {
        return bounding_rect;
    }

    void write(ecsWorldWriteCtx& w) override {
        w.write(order_index);
        w.write(translation);
        w.write(rotation);
        w.write(origin);
    }
    void read(ecsWorldReadCtx& r) override {
        r.read(order_index);
        r.read(translation);
        r.read(rotation);
        r.read(origin);
    }

    void onGui(ecsWorld* w, entity_id e) override {
        if(ImGui::DragInt("order index###guielemorder", &order_index)) {
            setOrderIndex(order_index);
        }
        if(ImGui::DragFloat2("translation###guielemtranslation", (float*)&translation)) {
            setTranslation(translation.x, translation.y);
        }
        if(ImGui::DragFloat("rotation###guielemrotation", &rotation)) {
            setRotation(rotation);
        }
    }
};
class ecsGuiNode : public ecsAttrib<ecsGuiNode> {
friend ecsRenderGui;
    gfxm::vec2 translation;
    gfxm::vec2 size;
public:

};
class ecsGuiContentLayout : public ecsAttrib<ecsGuiContentLayout> {
public:

};
class ecsGuiText : public ecsAttrib<ecsGuiText> {
    friend ecsRenderGui;
    
    std::shared_ptr<Font>   font;
    std::string             str = "Gui Text";
    uint16_t                face_height = 16;
    TEXT_ALIGN              alignment = TEXT_ALIGN_LEFT;
    float                   bb_width = .0f;
    float                   bb_height = .0f;


    GLuint vao = 0;
    GLuint vbuf = 0;
    GLuint uvbuf = 0;
    GLuint uvlookupbuf = 0;
    int vertex_count = 0;

    void rebuildBuffers() {
        if(!font || str.empty()) {
            return;
        }

        std::vector<gfxm::vec3> vertices;
        std::vector<gfxm::vec2> uv;
        std::vector<float>      uv_lookup_indices;
        float horiAdvance = .0f;
        float lineOffset = font->getLineHeight(face_height);
        gfxm::vec2 bb_min;
        gfxm::vec2 bb_max;

        int current_line_first_vertex = 0;
        int current_line_vertex_count = 0;
        float alignment_mul = .0f;
        if(alignment == TEXT_ALIGN_CENTER) {
            alignment_mul = 0.5f;
        } else if(alignment == TEXT_ALIGN_RIGHT) {
            alignment_mul = 1.0f;
        }
        float max_line_width = .0f;
        for(int i = 0; i < str.size(); ++i) {
            if(str[i] == '\n') {
                max_line_width = gfxm::_max(max_line_width, horiAdvance);
                for(int j = 0; j < current_line_vertex_count; ++j) {
                    vertices[j + current_line_first_vertex].x -= (int)(horiAdvance * alignment_mul);
                }
                current_line_first_vertex = vertices.size();
                current_line_vertex_count = 0;

                lineOffset += font->getLineHeight(face_height);
                horiAdvance = .0f;
                continue;
            }
            const auto& g = font->getGlyph(str[i], face_height);
            float y_ofs = g.height - g.bearingY;
            float x_ofs = g.bearingX;
            
            gfxm::vec3 sw(horiAdvance + x_ofs,           0 - y_ofs - lineOffset,        0);
            gfxm::vec3 ne(horiAdvance + g.width + x_ofs, g.height - y_ofs - lineOffset, 0);
            
            bb_min = gfxm::vec2(gfxm::_min(bb_min.x, sw.x), gfxm::_min(bb_min.y, sw.y));
            bb_max = gfxm::vec2(gfxm::_max(bb_max.x, ne.x), gfxm::_max(bb_max.y, ne.y));

            vertices.push_back(gfxm::vec3(sw.x, sw.y, 0));
            vertices.push_back(gfxm::vec3(ne.x, sw.y, 0));
            vertices.push_back(gfxm::vec3(sw.x, ne.y, 0));
            vertices.push_back(gfxm::vec3(ne.x, sw.y, 0));
            vertices.push_back(gfxm::vec3(ne.x, ne.y, 0));
            vertices.push_back(gfxm::vec3(sw.x, ne.y, 0));
            horiAdvance += g.horiAdvance / 64.0f;

            uv.push_back(gfxm::vec2(0, 1));
            uv.push_back(gfxm::vec2(1, 1));
            uv.push_back(gfxm::vec2(0, 0));
            uv.push_back(gfxm::vec2(1, 1));
            uv.push_back(gfxm::vec2(1, 0));
            uv.push_back(gfxm::vec2(0, 0));

            uv_lookup_indices.push_back(g.cache_id * 4);
            uv_lookup_indices.push_back(g.cache_id * 4 + 1);
            uv_lookup_indices.push_back(g.cache_id * 4 + 3);
            uv_lookup_indices.push_back(g.cache_id * 4 + 1);
            uv_lookup_indices.push_back(g.cache_id * 4 + 2);
            uv_lookup_indices.push_back(g.cache_id * 4 + 3);

            current_line_vertex_count += 6;
        }
        vertex_count = vertices.size();
        int vertex_stride = sizeof(float) * 3;

        max_line_width = gfxm::_max(max_line_width, horiAdvance);
        for(int j = 0; j < current_line_vertex_count; ++j) {
            vertices[j + current_line_first_vertex].x -= (int)(horiAdvance * alignment_mul);
        }
        current_line_first_vertex = vertices.size();
        current_line_vertex_count = 0;

        for(auto& v : vertices) {
            //v.y += lineOffset;
            v.x += max_line_width * alignment_mul;
        }

        bb_width  = max_line_width;
        bb_height = lineOffset;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbuf);
        glEnableVertexAttribArray(VERTEX_FMT::ENUM_TEXT::Position);
        glVertexAttribPointer(
            VERTEX_FMT::ENUM_TEXT::Position, 3, GL_FLOAT, GL_FALSE,
            vertex_stride, 0
        );
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(gfxm::vec3), vertices.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, uvbuf);
        glEnableVertexAttribArray(VERTEX_FMT::ENUM_TEXT::UV);
        glVertexAttribPointer(
            VERTEX_FMT::ENUM_TEXT::UV, 2, GL_FLOAT, GL_FALSE,
            sizeof(float) * 2, 0
        );
        glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(gfxm::vec2), uv.data(), GL_STREAM_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, uvlookupbuf);
        glEnableVertexAttribArray(VERTEX_FMT::ENUM_TEXT::TextUVLookup);
        glVertexAttribPointer(
            VERTEX_FMT::ENUM_TEXT::TextUVLookup, 1, GL_FLOAT, GL_FALSE, 
            sizeof(float), 0
        );
        glBufferData(GL_ARRAY_BUFFER, uv_lookup_indices.size() * sizeof(float), uv_lookup_indices.data(), GL_STREAM_DRAW);

        glBindVertexArray(0);

        getEntityHdl().signalUpdate<ecsGuiText>();
    }

public:
    ecsGuiText() {
        glGenBuffers(1, &vbuf);
        glGenBuffers(1, &uvbuf);
        glGenBuffers(1, &uvlookupbuf);

        glGenVertexArrays(1, &vao);
    }
    ~ecsGuiText() {
        if(vao) {
            glDeleteVertexArrays(1, &vao);
            glDeleteBuffers(1, &vbuf);
            glDeleteBuffers(1, &uvbuf);
            glDeleteBuffers(1, &uvlookupbuf);
        }
    }

    void setString(const std::string& s) {
        str = s;
        rebuildBuffers();
    }
    const std::string& getString() const {
        return str;
    }
    void setFont(const std::shared_ptr<Font>& f) {
        font = f;
        rebuildBuffers();
    }
    void setAlignment(TEXT_ALIGN align) {
        alignment = align;
        rebuildBuffers();
    }

    void write(ecsWorldWriteCtx& ctx) override {
        ctx.writeResource(font);
        ctx.writeStr(str);
        ctx.write(face_height);
        ctx.write<uint8_t>(alignment);
    }
    void read(ecsWorldReadCtx& ctx) override {
        font = ctx.readResource<Font>();
        str = ctx.readStr();
        face_height = ctx.read<uint16_t>();
        alignment = (TEXT_ALIGN)ctx.read<uint8_t>();

        rebuildBuffers();
    }
    void onGui(ecsWorld* world, entity_id e) override {
        imguiResourceTreeCombo("font", font, "ttf", [this](){
            setFont(font);
        });
        int h = face_height;
        if(ImGui::DragInt("face height", &h, 1, 10, 100)) {
            face_height = (uint16_t)h;
            rebuildBuffers();
        }
        if(ImGui::RadioButton("left###textleft", (int*)&alignment, (int)TEXT_ALIGN_LEFT)) {
            rebuildBuffers();
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("center###textcenter", (int*)&alignment, (int)TEXT_ALIGN_CENTER)) {
            rebuildBuffers();
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("right###textright", (int*)&alignment, (int)TEXT_ALIGN_RIGHT)) {
            rebuildBuffers();
        }
        
        char buf[2048];
        memset(buf, 0, 2048);
        if (!str.empty()) {
            memcpy(buf, str.data(), std::min((size_t)str.size(), (size_t)2048));
        }
        if(ImGui::InputTextMultiline("string", buf, 2048)) {
            setString(buf);
        }
    }
};
class ecsGuiImage : public ecsAttrib<ecsGuiImage> {
friend ecsRenderGui;

    std::shared_ptr<Texture2D> image;

public:

    void setImage(const std::shared_ptr<Texture2D>& img) {
        image = img;
        getEntityHdl().signalUpdate<ecsGuiImage>();
    }

    void write(ecsWorldWriteCtx& ctx) override {
        ctx.writeResource(image);
    }
    void read(ecsWorldReadCtx& ctx) override {
        image = ctx.readResource<Texture2D>();
    }

    void onGui(ecsWorld* world, entity_id e) override {
        imguiResourceTreeCombo("image###quadimage", image, "png", [this](){
            setImage(image);
        });
    }

};


#endif
