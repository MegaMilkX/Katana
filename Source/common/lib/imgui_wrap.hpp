#ifndef IMGUI_WRAP_H
#define IMGUI_WRAP_H

#include "imgui/imgui.h"

void ImGuiInit();
void ImGuiCleanup();
void ImGuiUpdate(float dt, int w, int h);
void ImGuiDraw();

#endif
