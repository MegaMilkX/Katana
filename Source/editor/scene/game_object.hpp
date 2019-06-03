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

class GameScene;
class Behavior;

class GameObject {
    friend GameScene;
public:
    GameObject();
    virtual ~GameObject();

    virtual void                        _onCreate() {
        onCreate();
    }
    virtual void                        onCreate() {}

    void                                setName(const std::string& name);
    const std::string&                  getName() const;

    GameScene*                          getScene();
    GameObject*                         getRoot();
    GameObject*                         getParent();

    TransformNode*                      getTransform();

    template<typename BHVR_T>
    void                                setBehavior();
    void                                setBehavior(rttr::type t);
    Behavior*                           getBehavior();
    void                                clearBehavior();

    GameObject*                         createChild();
    size_t                              childCount();
    GameObject*                         getChild(size_t i);
    GameObject*                         getChild(const std::string& name);
    GameObject*                         findObject(const std::string& name);
    void                                getAllObjects(std::vector<GameObject*>& result);
    void                                takeOwnership(GameObject* o);
    void                                remove(bool keep_children = false);
    void                                duplicate();

    std::shared_ptr<Attribute>          find(rttr::type component_type);
    std::shared_ptr<Attribute>          get(rttr::type component_type);
    template<typename COMPONENT_T>
    std::shared_ptr<COMPONENT_T>        get();
    template<typename COMPONENT_T>
    std::shared_ptr<COMPONENT_T>        find();
    size_t                              componentCount();
    std::shared_ptr<Attribute>          getById(size_t id);
    void                                deleteComponentById(size_t id);
    void                                deleteAllComponents();

    void                                refreshAabb();
    void                                setAabb(const gfxm::aabb& box);
    const gfxm::aabb&                   getAabb() const;

    virtual void                        serialize(std::ostream& out);
    virtual void                        deserialize(std::istream& in, size_t sz);

    virtual void                        onGui();
    bool                                serializeComponents(std::ostream& out);

    void                                write(out_stream& out);
    void                                read(in_stream& in);
private:
    std::shared_ptr<Attribute>          createComponent(rttr::type t);

    GameScene* scene = 0;
    std::string name = "Object";
    TransformNode transform;
    gfxm::aabb aabb = gfxm::aabb(
        gfxm::vec3(-.5f, -.5f, -.5f),
        gfxm::vec3(.5f, .5f, .5f)
    );
    GameObject* parent = 0;
    std::set<GameObject*> children;
    std::map<rttr::type, std::shared_ptr<Attribute>> components;
};

template<typename BHVR_T>
void GameObject::setBehavior() {
    setBehavior(rttr::type::get<T>());
}

template<typename COMPONENT_T>
std::shared_ptr<COMPONENT_T> GameObject::get() {
    return std::dynamic_pointer_cast<COMPONENT_T>(get(rttr::type::get<COMPONENT_T>()));
}
template<typename COMPONENT_T>
std::shared_ptr<COMPONENT_T> GameObject::find() {
    return std::dynamic_pointer_cast<COMPONENT_T>(find(rttr::type::get<COMPONENT_T>()));
}

#endif
