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
    SceneObject* skeleton_root = 0;
    std::vector<Transform*> bones;
    std::vector<gfxm::mat4> bind_pose;

    Skin() {}

    void onClone(Skin* other) {
        if(!other || !other->skeleton_root) return;

        LOG("OTHER SKEL ROOT: " << other->skeleton_root->getName());

        bind_pose = other->bind_pose;
        skeleton_root = getScene()->getRootObject()->findObject(other->skeleton_root->getName());
        if(skeleton_root) {
            bones.resize(other->bones.size());
            for(size_t i = 0; i < other->bones.size(); ++i) {
                if(!other->bones[i]) continue;
                bones[i] = skeleton_root->findObject(other->bones[i]->getObject()->getName())->get<Transform>();
            }
        }
    }

    virtual void serialize(std::ostream& out) {
        if(skeleton_root) {
            wt_string(out, skeleton_root->getName());
        } else {
            wt_string(out, "");
        }

        write(out, (uint32_t)bones.size());
        for(size_t i = 0; i < bones.size(); ++i) {
            if(bones[i]) {
                wt_string(out, bones[i]->getObject()->getName());
            } else {
                wt_string(out, "");
            }
            write(out, bind_pose[i]);
        }
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        std::string skel_root_name = rd_string(in);
        skeleton_root = getObject()->getScene()->getRootObject()->findObject(skel_root_name);

        uint32_t bone_count = read<uint32_t>(in);
        bones.resize(bone_count);
        bind_pose.resize(bone_count);
        for(uint32_t i = 0; i < bone_count; ++i) {
            std::string bone_name = rd_string(in);
            if(skeleton_root) {
                bones[i] = skeleton_root->findObject(bone_name)->get<Transform>();
            }
            bind_pose[i] = read<gfxm::mat4>(in);
        }
    }

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
