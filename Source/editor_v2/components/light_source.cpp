#include "light_source.hpp"

#include "../scene/controllers/render_controller.hpp"

OmniLight::~OmniLight() {
    getOwner()->getScene()->getController<RenderController>()->_unregOmniLight(this);
}
void OmniLight::onCreate() {
    getOwner()->getScene()->getController<RenderController>()->_regOmniLight(this);
}