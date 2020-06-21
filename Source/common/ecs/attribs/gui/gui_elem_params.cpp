#include "gui_elem_params.hpp"

#include "../gui_element.hpp"

#include "../../world.hpp"

void GuiElemTypeParamsBase::signalUpdate() {
    elem->getEntityHdl().signalUpdate<ecsGuiElement>();
}