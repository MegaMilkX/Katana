#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "component.hpp"
#include "../../common/gfxm.hpp"
#include "../scene/game_object.hpp"
#include "../../common/util/serialization.hpp"

class CmCamera : public ObjectComponent {
    RTTR_ENABLE(ObjectComponent)
public:
    enum PROJECTION {
        PERSPECTIVE,
        ORTHOGONAL
    };

    void setType(PROJECTION t) {
        ptype = t;
    }
    void setFov(float fov) {
        this->fov = fov;
    }
    float getFov() {
        return fov;
    }
    void setViewport(float w, float h) {
        vp = gfxm::vec2(w, h);
    }
    void setZNear(float znear) {
        this->znear = znear;
    }
    void setZFar(float zfar) {
        this->zfar = zfar;
    }

    const gfxm::mat4& getProjection(int w, int h) {
        if(ptype == PERSPECTIVE) {
            projection = gfxm::perspective(fov, (float)w/(float)h, znear, zfar);
        } else if(ptype == ORTHOGONAL) {
            projection = gfxm::ortho((float)0, vp.x, (float)0, vp.y, znear, zfar);
        }
        return projection;
    }

    const gfxm::mat4& getView() {
        view = gfxm::inverse(
            getOwner()->getTransform()->getWorldTransform()
        );
        return view;
    }

    virtual bool serialize(std::ostream& out) {
        write(out, projection);
        write(out, view);
        write<uint8_t>(out, ptype);
        write(out, vp);
        write(out, fov);
        write(out, znear);
        write(out, zfar);
        return true;
    }
    virtual bool deserialize(std::istream& in, size_t sz) {
        projection = read<gfxm::mat4>(in);
        view = read<gfxm::mat4>(in);
        ptype = (PROJECTION)read<uint8_t>(in);
        vp = read<gfxm::vec2>(in);
        fov = read<float>(in);
        znear = read<float>(in);
        zfar = read<float>(in);
        return true;
    }

private:
    gfxm::mat4 projection;
    gfxm::mat4 view;
    PROJECTION ptype = PERSPECTIVE;
    gfxm::vec2 vp = gfxm::vec2(640, 480);
    float fov = 0.90f;
    float znear = 0.01f;
    float zfar = 1000.0f;
};
STATIC_RUN(CmCamera) {
    rttr::registration::class_<CmCamera>("CmCamera")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

#endif
