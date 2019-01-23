#ifndef SKIN_HPP
#define SKIN_HPP

#include "component.hpp"
#include <vector>
#include "transform.hpp"

class Skin : public Component {
    CLONEABLE
public:
    std::vector<Transform*> bones;
    std::vector<gfxm::mat4> bind_pose;

    virtual void _editorGui() {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::Text("Bones");
        ImGui::BeginChild("Bones", ImVec2(0, 260), false, window_flags);
        for(auto t : bones) {
            ImGui::Text(t->getObject()->getName().c_str());
        }
        ImGui::EndChild();
    }
};
STATIC_RUN(Skin)
{
    rttr::registration::class_<Skin>("Skin")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
