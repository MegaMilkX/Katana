#include "light_source.hpp"

#include "../scene/controllers/render_controller.hpp"

OmniLight::~OmniLight() {
}
void OmniLight::onCreate() {
    getOwner()->getScene()->getController<RenderController>();
}