#ifndef MOTION_GUI_HPP
#define MOTION_GUI_HPP

#include "../common/resource/motion.hpp"
#include "../common/resource/anim_fsm.hpp"
#include "../common/resource/blend_tree.hpp"
#include "../common/resource/skeleton.hpp"

class DocMotion;

class MotionGuiBase {
protected:
    DocMotion* doc;
    std::string title;

public:
    MotionGuiBase(const std::string& title, DocMotion* doc)
    : title(title), doc(doc) {}

    const std::string& getTitle() const { return title; }

    virtual void drawGui(Editor* ed, float dt) = 0;
    virtual void drawToolbox(Editor* ed) = 0;
};


#endif
