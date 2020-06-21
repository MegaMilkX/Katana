#ifndef GUI_ELEM_PARAMS_HPP
#define GUI_ELEM_PARAMS_HPP


#include "../../../resource/font.hpp"
#include "../../../util/imgui_helpers.hpp"
#include "gl_text_buffer.hpp"


enum GUI_ELEM_TYPE {
    GUI_ELEM_TEXT,
    GUI_ELEM_IMAGE,
    GUI_ELEM_CONTAINER,

    GUI_ELEM_TYPE_COUNT
};
inline const char* getGuiElemTypeName(GUI_ELEM_TYPE type) {
    static const char* name = "UNKNOWN";
    switch(type) {
    case GUI_ELEM_TEXT:                 name = "Text"; break;
    case GUI_ELEM_IMAGE:                name = "Image"; break;
    case GUI_ELEM_CONTAINER:            name = "Container"; break;
    default: assert(false); break;
    };
    return name;
}
class ecsGuiElement;
class GuiElemTypeParamsBase {
    GUI_ELEM_TYPE type;
protected:
    ecsGuiElement* elem = 0;
public:
    GuiElemTypeParamsBase(GUI_ELEM_TYPE type, ecsGuiElement* elem)
    : type(type), elem(elem) {}
    virtual ~GuiElemTypeParamsBase() {}
    virtual GUI_ELEM_TYPE getType() const { return type; }
    virtual bool onGui() = 0;

    void signalUpdate();
};
template<GUI_ELEM_TYPE TYPE>
class GuiElemTypeParams : public GuiElemTypeParamsBase {
public:
    GuiElemTypeParams(ecsGuiElement* elem)
    : GuiElemTypeParamsBase(TYPE, elem) {}

    static GUI_ELEM_TYPE getTypeStatic() { return TYPE; }
};

class ecsRenderGui;
class GuiElemText : public GuiElemTypeParams<GUI_ELEM_TEXT> {
friend ecsRenderGui;
    std::shared_ptr<Font>   font;
    int                     face_height = 16;
    std::string             text;
    gfxm::vec4              color;
    TEXT_ALIGN              alignment = TEXT_ALIGN_LEFT;
    bool                    vertical_center = true;

    glTextBuffer            textBuffer;

public:
    GuiElemText(ecsGuiElement* elem)
    : GuiElemTypeParams<GUI_ELEM_TEXT>(elem) {}

    void setString(const std::string& s) {
        text = s;
        signalUpdate();
    }
    const std::string& getString() const {
        return text;
    }
    void setFont(const std::shared_ptr<Font>& f) {
        font = f;
        signalUpdate();
    }
    void setFaceHeight(int h) {
        face_height = h;
        signalUpdate();
    }
    void setAlignment(TEXT_ALIGN align) {
        alignment = align;
        signalUpdate();
    }
    void setVerticalMiddle(bool v) {
        vertical_center = v;
        signalUpdate();
    }

    bool onGui() override {
        bool touched = false;
        imguiResourceTreeCombo("font", font, "ttf", [this](){
            setFont(font);
        });
        int h = face_height;
        if(ImGui::DragInt("face height", &h, 1, 10, 100)) {
            setFaceHeight(h);
        }
        if(ImGui::RadioButton("left###textleft", (int*)&alignment, (int)TEXT_ALIGN_LEFT)) {
            setAlignment(alignment);
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("center###textcenter", (int*)&alignment, (int)TEXT_ALIGN_CENTER)) {
            setAlignment(alignment);
        }
        ImGui::SameLine();
        if(ImGui::RadioButton("right###textright", (int*)&alignment, (int)TEXT_ALIGN_RIGHT)) {
            setAlignment(alignment);
        }
        if(ImGui::Checkbox("vertical middle", &vertical_center)) {
            setVerticalMiddle(vertical_center);
        }
        
        char buf[2048];
        memset(buf, 0, 2048);
        if (!text.empty()) {
            memcpy(buf, text.data(), std::min((size_t)text.size(), (size_t)2048));
        }
        if(ImGui::InputTextMultiline("string", buf, 2048)) {
            setString(buf);
        }
        return touched;
    }
};
class GuiElemImage : public GuiElemTypeParams<GUI_ELEM_IMAGE> {
friend ecsRenderGui;
    std::shared_ptr<Texture2D> image;
    
public:
    GuiElemImage(ecsGuiElement* elem)
    : GuiElemTypeParams<GUI_ELEM_IMAGE>(elem) {}
    
    void setImage(const std::shared_ptr<Texture2D>& image) {
        this->image = image;
        signalUpdate();
    }
    
    bool onGui() override {
        bool touched = false;
        imguiResourceTreeCombo("image###elemimage", image, "png", [this, &touched](){
            setImage(image);
            touched = true;
        });
        return touched;
    }
};
class GuiElemContainer : public GuiElemTypeParams<GUI_ELEM_CONTAINER> {
public:
    GuiElemContainer(ecsGuiElement* elem)
    : GuiElemTypeParams<GUI_ELEM_CONTAINER>(elem) {}
    bool onGui() override {
        bool touched = false;
        return touched;
    }
};

inline GuiElemTypeParamsBase* createGuiElemData(GUI_ELEM_TYPE type, ecsGuiElement* elem) {
    GuiElemTypeParamsBase* ptr = 0;
    switch(type) {
    case GUI_ELEM_TEXT:                 ptr = new GuiElemText(elem); break;
    case GUI_ELEM_IMAGE:                ptr = new GuiElemImage(elem); break;
    case GUI_ELEM_CONTAINER:            ptr = new GuiElemContainer(elem); break;
    default: assert(false); break;
    };
    return ptr;
}


#endif
