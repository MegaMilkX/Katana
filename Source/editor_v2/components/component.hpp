#ifndef COMPONENT_HPP
#define COMPONENT_HPP

#include <rttr/type>
#include <rttr/registration>

#include "../editor_component_desc.hpp"

class GameObject;
class ObjectComponent {
    RTTR_ENABLE()

    friend GameObject;
public:
    virtual ~ObjectComponent();

    virtual void copy(ObjectComponent* other) {}

    GameObject* getOwner() { return owner; }

    virtual IEditorComponentDesc* _newEditorDescriptor() {
        return new EditorComponentDesc<ObjectComponent>(this);
    }
private:
    GameObject* owner = 0;
};

#endif
