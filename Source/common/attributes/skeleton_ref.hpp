#ifndef SKELETON_REF_HPP
#define SKELETON_REF_HPP

#include "attribute.hpp"

#include "../resource/skeleton.hpp"

#include "../util/imgui_helpers.hpp"

#include "../debug_draw.hpp"

class SkeletonRef : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    std::shared_ptr<Skeleton> skeleton;

    void debugDraw(DebugDraw* dd) {
        if(!skeleton) {
            return;
        }
        for(size_t i = 0; i < skeleton->boneCount(); ++i) {
            auto& b = skeleton->getBone(i);
            auto o = getOwner()->findObject(b.name);
            for(size_t j = 0; j < o->childCount(); ++j) {
                auto c = o->getChild(j);
                dd->line(o->getTransform()->getWorldPosition(), c->getTransform()->getWorldPosition(), gfxm::vec3(1,1,1), DebugDraw::DEPTH_DISABLE);
            }
        }
    }

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

#endif
