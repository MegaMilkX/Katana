#ifndef EDITOR_COMPONENT_DESC_HPP
#define EDITOR_COMPONENT_DESC_HPP

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"

class IEditorComponentDesc {
public:
    virtual ~IEditorComponentDesc() {}
    virtual void updateData() {}
    virtual void gui() = 0;
};

template<typename T>
class EditorComponentDesc : public IEditorComponentDesc {
public:
    EditorComponentDesc(T* c)
    : component(c) {}
    virtual void updateData() {}
    virtual void gui() {
        ImGui::Text("Component GUI not defined");
    }
private:
    T* component = 0;
};

#endif
