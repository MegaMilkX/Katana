#ifndef PLATFORM_HPP
#define PLATFORM_HPP

bool platformInit();
void platformCleanup();
bool platformIsShuttingDown();
void platformUpdate();
void platformSwapBuffers();

void* platformGetGlfwWindow();

#endif
