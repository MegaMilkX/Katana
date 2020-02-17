#ifndef ACTION_STATE_MACHINE_HPP
#define ACTION_STATE_MACHINE_HPP

#include "attribute.hpp"

#include "../resource/skeleton.hpp"
#include "../resource/motion.hpp"
#include "../resource/animation.hpp"

class ActionStateMachine : public Attribute {
    RTTR_ENABLE(Attribute)

    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<Motion> motion_ref;

    bool skeleton_nodes_dirty = true;
    std::vector<ktNode*> skeleton_nodes;
    std::vector<AnimSample> sample_buffer;

    Motion motion;
    int32_t current_action_id = -1;

    void resizeSampleBuffer();
    void makeGraphLocalCopy();

public:
    void update(float dt);

    void onGui() override;

    void write(SceneWriteCtx& out) override;
    void read(SceneReadCtx& in) override;

    const char* getIconCode() const override { return ICON_MDI_RUN_FAST; }
};

#endif
