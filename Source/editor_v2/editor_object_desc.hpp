#ifndef EDITOR_OBJECT_DESC_HPP
#define EDITOR_OBJECT_DESC_HPP

#include "../common/lib/imgui_wrap.hpp"
#include "../common/lib/imgui/imgui_internal.h"

class IEditorObjectDesc {
public:
    virtual ~IEditorObjectDesc() {}
    virtual void updateData() {}
    virtual void gui() = 0;
};

template<typename T>
class EditorObjectDesc : public IEditorObjectDesc{
public:
    EditorObjectDesc(T* o)
    : object(o) {}
    virtual void updateData() {}
    virtual void gui() {
        ImGui::Text("Object GUI");
    }
private:
    T* object = 0;
};

#endif
