#ifndef SCENE_CONTROLLER_HPP
#define SCENE_CONTROLLER_HPP

#include <rttr/type>
#include <rttr/registration>
#include "../../common/util/static_run.h"

#include "../../common/debug_draw.hpp"

#include "../../common/util/data_stream.hpp"
#include "../../common/util/data_reader.hpp"
#include "../../common/util/data_writer.hpp"

enum FRAME_PRIORITY {
    FRAME_PRIORITY_FIRST,
    FRAME_PRIORITY_ANIM = 1000,
    FRAME_PRIORITY_CONSTRAINT = 2000,
    FRAME_PRIORITY_ACTORS = 3000,
    FRAME_PRIORITY_DYNAMICS = 4000,
    FRAME_PRIORITY_DRAW = 5000,
    FRAME_PRIORITY_LAST = 10000
};

struct SceneCtrlInfo {
    bool auto_update;
    int priority;
};

class GameScene;
class SceneController {
    RTTR_ENABLE()
public:
    virtual ~SceneController() {}

    virtual SceneCtrlInfo getInfo() const {
        return SceneCtrlInfo{ false, FRAME_PRIORITY_ACTORS };
    }

    virtual void copy(SceneController& other) {}

    virtual void init(GameScene* scn) {}

    virtual void onStart() {

    }

    virtual void onUpdate() {
        
    }

    virtual void onEnd() {

    }

    virtual void debugDraw(DebugDraw& dd) {}

    virtual void onGui() {}

    virtual void serialize(out_stream& out) {}
    virtual void deserialize(in_stream& in) {}
};

#endif
