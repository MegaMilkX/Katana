#ifndef RENDERABLE_BASE_HPP
#define RENDERABLE_BASE_HPP

#include "attribute.hpp"
#include "../draw_list.hpp"

class RenderableBase : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual ~RenderableBase() {}
    virtual void addToDrawList(DrawList& dl) = 0;

    bool requiresTransformCallback() const override { return true; }
};

#endif
