#ifndef SKIN_HPP
#define SKIN_HPP

#include "component.hpp"
#include <vector>
#include "transform.hpp"

#include "util/serialization.hpp"

class Skin : public Component {
    CLONEABLE
public:
    struct Segment {
        SceneObject* skeleton_root = 0;
        std::vector<Transform*> bones;
        std::vector<gfxm::mat4> bind_pose;
    };

    std::vector<Segment> segments;

    Segment& getSegment(size_t i) {
        if(i >= segments.size()) {
            segments.resize(i + 1);
        }
        return segments[i];
    }

    size_t segmentCount() const {
        return segments.size();
    }

    void onClone(Skin* other) {
        segments.resize(other->segments.size());
        for(size_t i = 0; i < other->segments.size(); ++i) {
            segments[i].bind_pose = other->segments[i].bind_pose;
            segments[i].skeleton_root = getScene()->getRootObject()->findObject(other->segments[i].skeleton_root->getName());
            if(segments[i].skeleton_root) {
                segments[i].bones.resize(other->segments[i].bones.size());
                for(size_t j = 0; j < other->segments[i].bones.size(); ++j) {
                    if(!other->segments[i].bones[j]) continue;
                    segments[i].bones[j] = segments[i].skeleton_root->findObject(other->segments[i].bones[j]->getObject()->getName())->get<Transform>();
                }
            }
        }
    }

    virtual void serialize(std::ostream& out) {
        write<uint32_t>(out, segmentCount());
        for(size_t i = 0; i < segmentCount(); ++i) {
            if(getSegment(i).skeleton_root) {
                wt_string(out, getSegment(i).skeleton_root->getName());
            } else {
                wt_string(out, "");
            }

            write(out, (uint32_t)getSegment(i).bones.size());
            for(size_t j = 0; j < getSegment(i).bones.size(); ++j) {
                if(getSegment(i).bones[j]) {
                    wt_string(out, getSegment(i).bones[j]->getObject()->getName());
                } else {
                    wt_string(out, "");
                }
                write(out, getSegment(i).bind_pose[j]);
            }    
        }
    }
    virtual void deserialize(std::istream& in, size_t sz) {
        uint32_t seg_count = read<uint32_t>(in);
        for(uint32_t i = 0; i < seg_count; ++i) {
            std::string skel_root_name = rd_string(in);
            getSegment(i).skeleton_root = getObject()->getScene()->getRootObject()->findObject(skel_root_name);

            uint32_t bone_count = read<uint32_t>(in);
            getSegment(i).bones.resize(bone_count);
            getSegment(i).bind_pose.resize(bone_count);
            for(uint32_t j = 0; j < bone_count; ++j) {
                std::string bone_name = rd_string(in);
                if(getSegment(i).skeleton_root) {
                    getSegment(i).bones[j] = getSegment(i).skeleton_root->findObject(bone_name)->get<Transform>();
                }
                getSegment(i).bind_pose[j] = read<gfxm::mat4>(in);
            }
        }
    }

    virtual void _editorGui() {
        /*
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::Text("Bones");
        ImGui::BeginChild("Bones", ImVec2(0, 260), false, window_flags);
        for(auto t : bones) {
            ImGui::Text(t->getObject()->getName().c_str());
        }
        ImGui::EndChild();
        */
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
