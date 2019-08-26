#ifndef APPLICATION_STATE_HPP
#define APPLICATION_STATE_HPP

class AppState {
public:
    virtual void onInit() {}
    virtual void onCleanup() {}

    virtual void onUpdate() {}
    virtual void onGui(float dt) {}
    virtual void onRender(int w, int h) {}
};

#endif
