#ifndef SKIN_HPP
#define SKIN_HPP

#include "component.hpp"
#include <vector>
#include "transform.hpp"

#include "util/serialization.hpp"

class Skin : public Component {
    CLONEABLE
public:
    // TODO: Change bone reference method?
    std::vector<size_t> bones;
    std::vector<gfxm::mat4> bind_pose;

    Skin() {}

    void onClone(Skin* other) {
        
    }

    virtual void serialize(std::ostream& out) {
        write(out, (uint32_t)bones.size());
        for(size_t i = 0; i < bones.size(); ++i) {
            write(out, (uint32_t)bones[i]);
            write(out, bind_pose[i]);
        }
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        uint32_t bone_count = read<uint32_t>(in);
        bones.resize(bone_count);
        bind_pose.resize(bone_count);
        for(uint32_t i = 0; i < bone_count; ++i) {
            bones[i] = read<uint32_t>(in);
            bind_pose[i] = read<gfxm::mat4>(in);
        }
    }

    virtual void _editorGui() {
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::Text("Bones");
        ImGui::BeginChild("Bones", ImVec2(0, 260), false, window_flags);
        for(auto t : bones) {
            ImGui::Text(getScene()->getComponent<Transform>(t)->getObject()->getName().c_str());
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
