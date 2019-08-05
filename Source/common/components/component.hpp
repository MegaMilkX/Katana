#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <rttr/type>
#include <rttr/registration>
#include "../../common/util/static_run.h"

#include "../../common/gfxm.hpp"

#include "../../common/util/data_stream.hpp"
#include "../../common/util/data_writer.hpp"

#include "../../common/util/log.hpp"

#include "../../common/lib/imgui_wrap.hpp"

#include "../debug_draw.hpp"
#include "../gui_viewport.hpp"

#include "../../common/util/materialdesign_icons.hpp"

#include "attrib_type_lib.hpp"

class ktNode;
class GameScene;
class Attribute {
    RTTR_ENABLE()

    friend GameScene;
    friend ktNode;
public:
    virtual ~Attribute();

    virtual rttr::type getRequiredOwnerType();

    // Basically a constructor for components
    virtual void onCreate();

    virtual void copy(Attribute* other);

    ktNode* getOwner();
    void resetAttribute();

    virtual bool buildAabb(gfxm::aabb& out);

    virtual void onGui();
    virtual void onGizmo(GuiViewport& vp);
    virtual const char* getIconCode() const { return ""; }

    virtual bool serialize(out_stream& out);
    virtual bool deserialize(in_stream& in, size_t sz);
private:
    ktNode* owner = 0;
};

template<typename T>
class AttributeCopyable : public Attribute {
    RTTR_ENABLE(Attribute)
public:
    virtual void copy(Attribute* other) {
        if(other->get_type() != get_type()) {
            LOG_WARN("Can't copy from " << other->get_type().get_name().to_string() << " to " <<
                get_type().get_name().to_string());
            return;
        }
        T* o = (T*)other;
        copy(*o);
    }
    virtual void copy(const T& other) = 0;
};

#define REG_ATTRIB(TYPE, NAME, CATEGORY) \
STATIC_RUN(TYPE) { \
    rttr::registration::class_<TYPE>(#NAME) \
        .constructor<>()( \
            rttr::policy::ctor::as_raw_ptr \
        ); \
    getAttribTypeLib().add(#CATEGORY, rttr::type::get<TYPE>()); \
}

#endif
