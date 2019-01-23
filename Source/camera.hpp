#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "component.hpp"
#include "gfxm.hpp"
#include "transform.hpp"

#include "util/log.hpp"

class Camera : public Component {
    CLONEABLE
public:
    enum PROJECTION_TYPE {
        PERSPECTIVE,
        ORTHOGONAL
    };

    Camera()
    : type(PERSPECTIVE), fov(0.90f), vp(640, 480), znear(0.1f), zfar(1000.0f) {
    }
    ~Camera();

    void setType(PROJECTION_TYPE type) {
        this->type = type;
    }
    void setFov(float fov) {
        this->fov = fov;
    }
    float getFov() {
        return fov;
    }
    void setViewportSize(float w, float h) {
        vp.x = w;
        vp.y = h;
    }
    void setZNear(float znear) {
        this->znear = znear;
    }
    void setZFar(float zfar) {
        this->zfar = zfar;
    }

    const gfxm::mat4& getProjection(int w, int h) {
        if(type == PERSPECTIVE) {
            projection = gfxm::perspective(fov, (float)w/(float)h, znear, zfar);
        } else if(type == ORTHOGONAL) {
            projection = gfxm::ortho((float)0, vp.x, (float)0, vp.y, znear, zfar);
        }

        return projection;
    }

    const gfxm::mat4& getView();

    void makeCurrent();

    virtual void _editorGui() {
        int vbutton = (int)type;
        ImGui::RadioButton("Perspective", &vbutton, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Orthogonal", &vbutton, 1);
        ImGui::Separator();
        type = (PROJECTION_TYPE)vbutton;
        
        if(type == PERSPECTIVE) {
            ImGui::DragFloat("fov", &fov, 0.01f, 0.01f, 1.0f);
            ImGui::DragFloat2("viewport", (float*)&vp, 1.0f, 1.0f, 10000.0f);
        } else if(type == ORTHOGONAL) {
            ImGui::DragFloat2("viewport", (float*)&vp, 1.0f, 1.0f, 10000.0f);
        }

        ImGui::DragFloat("znear", &znear, 0.01f, 0.0001f, 1.0f);
        ImGui::DragFloat("zfar", &zfar, 0.01f, 1.0f, 10000.0f);
    }
private:
    gfxm::mat4 projection;
    gfxm::mat4 view;
    PROJECTION_TYPE type;
    gfxm::vec2 vp;
    float fov;
    float znear;
    float zfar;
};
STATIC_RUN(Camera)
{
    rttr::registration::class_<Camera>("Camera")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
