#include "render_environment.hpp"

#include "../lib/imgui_gradient.hpp"

static ImGradient gradient_;
static ImGradientMark* draggingMark = nullptr;
static ImGradientMark* selectedMark = nullptr;

void RenderEnvironment::onGui() {
    if(ImGui::GradientEditor(&gradient_, draggingMark, selectedMark)) {
        gradient = curve<gfxm::vec3>();
        auto& list = gradient_.getMarks();
        for(auto node : list) {
            gradient[node->position] = gfxm::vec3(
                node->color[0],
                node->color[1],
                node->color[2]
            );
        }
    }
}

void RenderEnvironment::write(SceneWriteCtx& dw) {
    dw.write(gradient.get_keyframes());
}
void RenderEnvironment::read(SceneReadCtx& dw) {
    std::vector<keyframe<gfxm::vec3>> keyframes = dw.readArray<keyframe<gfxm::vec3>>();
    gradient = curve<gfxm::vec3>();
    gradient.set_keyframes(keyframes);
}