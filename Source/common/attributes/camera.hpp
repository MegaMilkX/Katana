#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "attribute.hpp"
#include "../../common/gfxm.hpp"
#include "../scene/node.hpp"
#include "../../common/util/serialization.hpp"

class Camera : public Attribute {
    RTTR_ENABLE(Attribute)
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

    virtual bool serialize(out_stream& out) {
        out.write(projection);
        out.write(view);
        out.write<uint8_t>(ptype);
        out.write(vp);
        out.write(fov);
        out.write(znear);
        out.write(zfar);
        return true;
    }
    virtual bool deserialize(in_stream& in, size_t sz) {
        projection = in.read<gfxm::mat4>();
        view = in.read<gfxm::mat4>();
        ptype = (PROJECTION)in.read<uint8_t>();
        vp = in.read<gfxm::vec2>();
        fov = in.read<float>();
        znear = in.read<float>();
        zfar = in.read<float>();
        return true;
    }

    virtual const char* getIconCode() const { return ICON_MDI_VIDEO_VINTAGE; }
private:
    gfxm::mat4 projection;
    gfxm::mat4 view;
    PROJECTION ptype = PERSPECTIVE;
    gfxm::vec2 vp = gfxm::vec2(640, 480);
    float fov = gfxm::radian(45.0f);
    float znear = 0.01f;
    float zfar = 1000.0f;
};
REG_ATTRIB(Camera, Camera, Rendering);

#endif
