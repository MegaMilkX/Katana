#ifndef GUI_ATTRIB_HPP
#define GUI_ATTRIB_HPP

#include <vector>
#include "../../../gfxm.hpp"
#include "../../../resource/texture2d.h"

enum GUI_ATTRIB {
    GUI_ATTRIB_FIRST,

    GUI_ATTRIB_COLOR = GUI_ATTRIB_FIRST,
    GUI_ATTRIB_BG_COLOR,
    GUI_ATTRIB_BG_IMAGE,
    GUI_ATTRIB_MARGIN,
    GUI_ATTRIB_PADDING,

    GUI_ATTRIB_COUNT
};
inline const char* getGuiAttribName(GUI_ATTRIB a) {
    const char* name = "UNKNOWN";
    switch(a) {
    case GUI_ATTRIB_COLOR:      name = "Color"; break;
    case GUI_ATTRIB_BG_COLOR:   name = "Background color"; break;
    case GUI_ATTRIB_BG_IMAGE:   name = "Background image"; break;
    case GUI_ATTRIB_MARGIN:     name = "Margin"; break;
    case GUI_ATTRIB_PADDING:    name = "Padding"; break;
    default: assert(false);
    };
    return name;
}

class guiAttribBase {
public:
    virtual ~guiAttribBase() {}
    virtual int getTypeId() = 0;
};
template<int TYPE_ID>
class guiAttrib : public guiAttribBase {
public:
    static int type_id;
    int getTypeId() override { return type_id; }
};
template<int TYPE_ID>
int guiAttrib<TYPE_ID>::type_id = TYPE_ID;

struct guiColor : public guiAttrib<GUI_ATTRIB_COLOR> {
    gfxm::vec4 color;
};
struct guiBgColor : public guiAttrib<GUI_ATTRIB_BG_COLOR> {
    gfxm::vec4 color;
};
struct guiBgImage : public guiAttrib<GUI_ATTRIB_BG_IMAGE> {
    std::shared_ptr<Texture2D> image;
};
struct guiMargin : public guiAttrib<GUI_ATTRIB_MARGIN> {
    gfxm::vec4 margin;
};
struct guiPadding : public guiAttrib<GUI_ATTRIB_PADDING> {
    gfxm::vec4 padding;
};

inline guiAttribBase* createGuiAttrib(GUI_ATTRIB a) {
    guiAttribBase* ptr = 0;
    switch(a) {
    case GUI_ATTRIB_COLOR:      ptr = new guiColor; break;
    case GUI_ATTRIB_BG_COLOR:   ptr = new guiBgColor; break;
    case GUI_ATTRIB_BG_IMAGE:   ptr = new guiBgImage; break;
    case GUI_ATTRIB_MARGIN:     ptr = new guiMargin; break;
    case GUI_ATTRIB_PADDING:    ptr = new guiPadding; break;
    default: assert(false);
    };
    return ptr;
}


#endif
