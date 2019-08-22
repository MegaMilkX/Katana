#ifndef ACTION_STATE_MACHINE_HPP
#define ACTION_STATE_MACHINE_HPP

#include "attribute.hpp"

#include "../resource/skeleton.hpp"
#include "../resource/action_graph.hpp"
#include "../resource/animation.hpp"

class ActionStateMachine : public Attribute {
    RTTR_ENABLE(Attribute)

    struct AnimMapping {
        std::string alias;
        std::shared_ptr<Animation> anim;
        std::vector<int32_t> bone_mapping;
    };

    std::shared_ptr<Skeleton> skeleton;
    std::shared_ptr<ActionGraph> graph;
    std::vector<AnimMapping> anim_mappings;

    bool skeleton_nodes_dirty = true;
    std::vector<ktNode*> skeleton_nodes;
    std::vector<AnimSample> sample_buffer;

    void buildAnimSkeletonMappings();
    void resizeSampleBuffer();

public:
    void update(float dt);

    void onGui() override;

    void write(SceneWriteCtx& out) override;
    void read(SceneReadCtx& in) override;

    const char* getIconCode() const override { return ICON_MDI_RUN_FAST; }
};
REG_ATTRIB(ActionStateMachine, ActionStateMachine, Animation);

#endif
