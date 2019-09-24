#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "../config.hpp"

struct PlatformStartupParams {
    bool hide_window;
};

Config& platformGetConfig();

bool platformInit(PlatformStartupParams* params = 0);
void platformCleanup();
bool platformIsShuttingDown();
void platformUpdate(float dt);
void platformSwapBuffers();

void platformGetViewportSize(unsigned& width, unsigned& height);
void platformGetMousePos(unsigned& x, unsigned& y);
void platformMouseSetEnabled(bool);
void* platformGetGlfwWindow();

#endif
