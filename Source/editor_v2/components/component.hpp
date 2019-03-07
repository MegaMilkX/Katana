#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <rttr/type>
#include <rttr/registration>

#include "../editor_component_desc.hpp"

#include "../../common/gfxm.hpp"

class GameObject;
class ObjectComponent {
    RTTR_ENABLE()

    friend GameObject;
public:
    virtual ~ObjectComponent();

    // Basically a constructor for components
    virtual void onCreate();

    virtual void copy(ObjectComponent* other);

    GameObject* getOwner();

    virtual bool buildAabb(gfxm::aabb& out);

    virtual bool serialize(std::ostream& out);
    virtual bool deserialize(std::istream& in, size_t sz);

    virtual IEditorComponentDesc* _newEditorDescriptor();
private:
    GameObject* owner = 0;
};

#endif
