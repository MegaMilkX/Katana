#ifndef APP_STATE_HPP
#define APP_STATE_HPP

#include "scene/scene.hpp"

class AppState {
public:
    virtual void onInit() {}
    virtual void onCleanup() {}

    virtual void onUpdate() {}
    virtual void onGui() {}
    virtual void onRender() {}
private:
    std::shared_ptr<Scene3> scene;
};

#endif
