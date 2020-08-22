#include "katana_impl.hpp"

#include "platform/platform.hpp"

#include "scene/controllers/render_controller.hpp"
#include "scene/controllers/anim_controller.hpp"
#include "scene/controllers/audio_controller.hpp"
#include "scene/controllers/constraint_ctrl.hpp"
#include "scene/controllers/dynamics_ctrl.hpp"

#include "../common/debug_overlay/debug_overlay.hpp"

static float sFrameDelta = .0f;
static float sTime = .0f;

KatanaImpl::KatanaImpl() {
    vp.init(640, 480);
}

void KatanaImpl::run(ktGameMode* sess) {
    _session = sess;
    _session->_start();
}
void KatanaImpl::stop() {
    _session->_cleanup();
}

float KatanaImpl::getTime() { return sTime; }
float KatanaImpl::getDt() { return sFrameDelta; }

void KatanaImpl::update() {
    if(!_session) return;
    _session->_update(sFrameDelta);
    _session->_render();

    debugOverlayDraw(1280, 720);
}

void KatanaImpl::update_time(float delta_time) {
    sFrameDelta = delta_time;
    sTime += delta_time;
}