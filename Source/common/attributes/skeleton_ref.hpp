#ifndef SKELETON_REF_HPP
#define SKELETON_REF_HPP

#include "attribute.hpp"

#include "../resource/skeleton.hpp"

#include "../util/imgui_helpers.hpp"

class SkeletonRef : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    std::shared_ptr<Skeleton> skeleton;

    void onGui() override {
        imguiResourceTreeCombo("skeleton", skeleton, "skl", [](){

        });
    }

    void write(SceneWriteCtx& o) override {
        o.write(skeleton);
    }
    void read(SceneReadCtx& in) override {
        skeleton = in.readResource<Skeleton>();
    }
};
REG_ATTRIB(SkeletonRef, SkeletonRef, Character);

#endif
