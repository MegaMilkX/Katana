#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP

#include <rttr/type>
#include <rttr/registration>

#include <set>
#include <memory>

#include "../../common/util/log.hpp"
#include "../../common/util/static_run.h"

#include "../transform.hpp"

#include "../components/component.hpp"

#include "../editor_object_desc.hpp"

class GameObject;
class EditorGameObjectDesc : public IEditorObjectDesc {
public:
    EditorGameObjectDesc(GameObject* o);
    virtual void updateData();
    virtual void gui();
private:
    GameObject* object = 0;
    gfxm::vec3 t;
    gfxm::vec3 r;
    gfxm::vec3 s = gfxm::vec3(1.0f, 1.0f, 1.0f);
};

class GameScene;
class GameObject {
    RTTR_ENABLE()

    friend GameScene;
public:
    GameObject();

    void                                setUid(uint64_t uid);
    uint64_t                            getUid() const;

    void                                setName(const std::string& name);
    const std::string&                  getName() const;

    GameScene*                          getScene();
    GameObject*                         getRoot();

    TransformNode*                      getTransform();

    virtual void                        copy(GameObject* other);
    void                                copyRecursive(GameObject* o);
    void                                copyComponents(GameObject* other);
    void                                copyComponentsRecursive(GameObject* other);

    std::shared_ptr<GameObject>         createChild();
    std::shared_ptr<GameObject>         createChild(rttr::type t);
    template<typename T>
    std::shared_ptr<T>                  createChild();
    size_t                              childCount();
    std::shared_ptr<GameObject>         getChild(size_t i);
    std::shared_ptr<GameObject>         getChild(const std::string& name);
    GameObject*                         findObject(const std::string& name);

    std::shared_ptr<ObjectComponent>    find(rttr::type component_type);
    std::shared_ptr<ObjectComponent>    get(rttr::type component_type);
    template<typename COMPONENT_T>
    std::shared_ptr<COMPONENT_T>        get();
    template<typename COMPONENT_T>
    std::shared_ptr<COMPONENT_T>        find();
    size_t                              componentCount();
    std::shared_ptr<ObjectComponent>    getById(size_t id);

    virtual IEditorObjectDesc*          _newEditorObjectDesc();
private:
    std::shared_ptr<ObjectComponent>    createComponent(rttr::type t);

    uint64_t uid;
    GameScene* scene = 0;
    std::string name = "Object";
    TransformNode transform;
    GameObject* parent = 0;
    std::set<std::shared_ptr<GameObject>> children;
    std::map<rttr::type, std::shared_ptr<ObjectComponent>> components;
};
STATIC_RUN(GameObject) {
    rttr::registration::class_<GameObject>("GameObject")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

template<typename T>
std::shared_ptr<T> GameObject::createChild() {
    return std::dynamic_pointer_cast<T>(createChild(rttr::type::get<T>()));
}

template<typename COMPONENT_T>
std::shared_ptr<COMPONENT_T> GameObject::get() {
    return std::dynamic_pointer_cast<COMPONENT_T>(get(rttr::type::get<COMPONENT_T>()));
}
template<typename COMPONENT_T>
std::shared_ptr<COMPONENT_T> GameObject::find() {
    return std::dynamic_pointer_cast<COMPONENT_T>(find(rttr::type::get<COMPONENT_T>()));
}

inline EditorGameObjectDesc::EditorGameObjectDesc(GameObject* o)
: object(o) {
    updateData();
}
inline void EditorGameObjectDesc::updateData() {
    t = object->getTransform()->getPosition();
    r = object->getTransform()->getEulerAngles();
    s = object->getTransform()->getScale();
}
inline void EditorGameObjectDesc::gui() {
    char buf[256];
    memset(buf, 0, 256);
    memcpy(buf, object->getName().c_str(), object->getName().size());
    if(ImGui::InputText("Name", buf, 256)) {
        object->setName(buf);
    }
    ImGui::Separator();
    if(ImGui::DragFloat3("Translation", (float*)&t, 0.001f)) {
        object->getTransform()->setPosition(t);
    }
    if(ImGui::DragFloat3("Rotation", (float*)&r, 0.001f)) {
        object->getTransform()->setRotation(r);
    }
    if(ImGui::DragFloat3("Scale", (float*)&s, 0.001f)) {
        object->getTransform()->setScale(s);
    }
}

#endif
