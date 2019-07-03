#ifndef SCENE_CONTROLLER_HPP
#define SCENE_CONTROLLER_HPP

#include <rttr/type>
#include <rttr/registration>
#include "../../common/util/static_run.h"

#include "../../common/debug_draw.hpp"

#include "../../common/util/data_stream.hpp"
#include "../../common/util/data_reader.hpp"
#include "../../common/util/data_writer.hpp"

#include "../components/component.hpp"

enum FRAME_PRIORITY {
    FRAME_PRIORITY_FIRST,
    FRAME_PRIORITY_ANIM = 1000,
    FRAME_PRIORITY_DYNAMICS = 2000,
    FRAME_PRIORITY_CONSTRAINT = 3000,
    FRAME_PRIORITY_ACTORS = 4000,
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

    virtual void attribCreated(rttr::type t, Attribute* attr) {}
    virtual void attribDeleted(rttr::type t, Attribute* attr) {}

    virtual void debugDraw(DebugDraw& dd) {}

    virtual void onGui() {}

    virtual void serialize(out_stream& out) {}
    virtual void deserialize(in_stream& in) {}
};

template<typename ...Ts>
class SceneControllerEventFilter;

template<typename TA>
class SceneControllerEventFilter<TA> : public SceneController {
public:
    virtual void attribCreated(rttr::type t, Attribute* attr) {
        if(t == rttr::type::get<TA>()) {
            onAttribCreated((TA*)attr);
        }
    }
    virtual void attribDeleted(rttr::type t, Attribute* attr) {
        if(t == rttr::type::get<TA>()) {
            onAttribRemoved((TA*)attr);
        }
    }
    
    virtual void onAttribCreated(TA* attrib) {}
    virtual void onAttribRemoved(TA* attrib) = 0;
};

template<typename TA, typename ...Ts>
class SceneControllerEventFilter<TA, Ts...> : public SceneControllerEventFilter<Ts...> {
public:
    virtual void attribCreated(rttr::type t, Attribute* attr) {
        SceneControllerEventFilter<Ts...>::attribCreated(t, attr);
        if(t == rttr::type::get<TA>()) {
            onAttribCreated((TA*)attr);
        }
    }
    virtual void attribDeleted(rttr::type t, Attribute* attr) {
        SceneControllerEventFilter<Ts...>::attribDeleted(t, attr);
        if(t == rttr::type::get<TA>()) {
            onAttribRemoved((TA*)attr);
        }
    }
    
    virtual void onAttribCreated(TA* attrib) {}
    virtual void onAttribRemoved(TA* attrib) = 0;
};

#endif
