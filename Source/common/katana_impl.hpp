#ifndef KATANA_IMPL_HPP
#define KATANA_IMPL_HPP

#include "../katana/katana.hpp"
#include "render_viewport.hpp"
#include "renderer.hpp"

class KatanaImpl : public KatanaApi {
    ktGameMode* _session = 0;
    RenderViewport vp;
    RendererPBR renderer;

    float frameDelta = .0f;
    float time = .0f;
public:
    KatanaImpl();

    virtual void run(ktGameMode* sess);
    virtual void stop();

    virtual float getTime();
    virtual float getDt();

    void update();

    void update_time(float delta_time);
};

#endif
