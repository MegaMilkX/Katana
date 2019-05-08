#ifndef TEST_STATE_HPP
#define TEST_STATE_HPP

#include "app_state/app_state.hpp"
#include "scene/scene.hpp"

#include "../common/debug_draw.hpp"
#include "../common/lib/imgui_wrap.hpp"

class MyAttr : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    std::string test;
};
STATIC_RUN(MyAttr) {
    reg_type<MyAttr>("MyAttr");
}

#define REG_ATTRIB(ATTR) STATIC_RUN(ATTR) { \
    reg_type<ATTR>(#ATTR); \
}

class TestState : public AppState {
public:
    virtual void onInit() {
        dd.init();

        scene = Scene3::create();

        scene->createObject("MyCoolObject");
        scene->createObject("SomeNode");
        auto o = scene->createObject("Yay");

        o->getAttrib<MyAttr>();
        o->getAttrib<MyAttr>()->test = "lol";
        o->getAttrib<MyAttr>()->test = "lol2";

        scene->logStats();
    }
    virtual void onUpdate() {
    }
    virtual void onGui() {
        ImGui::Begin("Object inspector");
        ImGui::Text("Attributes");
        if(ImGui::BeginCombo("Behavior", "<null>")) {
            ImGui::Selectable("<null>", false);
            ImGui::Selectable("Character", false);
            ImGui::Selectable("TpsCamera", false);
            ImGui::EndCombo();
        }
        ImGui::End();
    }
    virtual void onRender() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        dd.clear();
        dd.point(gfxm::vec3(.0f, .0f, .0f), gfxm::vec3(1.0f, .0f, .0f));
        dd.gridxz(gfxm::vec3(-10,-10,-10), gfxm::vec3(10,10,10), 1, gfxm::vec3(0.5f, 0.5f, 0.5f));
        
        gfxm::mat4 view = gfxm::inverse(gfxm::translate(gfxm::mat4(1.0f), gfxm::vec3(.2f, 0.5f, 2.0f)));
        gfxm::mat4 proj = gfxm::perspective(1.4f, 16/9.0f, 0.01f, 100.0f);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0,0,1280,720);
        dd.draw(proj, view);
    }
private:
    std::shared_ptr<Scene3> scene;
    DebugDraw dd;
};

#endif
