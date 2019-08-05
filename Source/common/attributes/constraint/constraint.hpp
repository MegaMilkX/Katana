#ifndef CONSTRAINT_HPP
#define CONSTRAINT_HPP

#include <rttr/type>
#include <rttr/registration>
#include "../../../common/util/static_run.h"

#include "../../transform.hpp"

#include "../../scene/game_scene.hpp"

#include "../../../common/util/data_stream.hpp"

namespace Constraint {

class Constraint {
    RTTR_ENABLE()
public:
    virtual ~Constraint() {}

    virtual void copy(Constraint* other) {}

    void setScene(GameScene* s) { scene = s; }
    GameScene* getScene() { return scene; }

    virtual void update(ktNode* t) = 0;
    virtual void onGui() = 0;

    virtual void serialize(out_stream& out) = 0;
    virtual void deserialize(in_stream& in) = 0;
protected:
    GameScene* scene;
};

}

#endif
