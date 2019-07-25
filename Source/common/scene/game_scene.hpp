#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../resource/resource.h"

#include "game_object.hpp"
#include "scene_event_mgr.hpp"

#include "../components/camera.hpp"

#include "../../common/debug_draw.hpp"

#include "actor_object.hpp"
#include "scene_controller.hpp"
#include "../serializable.hpp"

class GameScene : public Resource {
public:
    GameScene();
    ~GameScene();

    void clear();

    GameObject* getRoot();

    std::vector<GameObject*> findObjectsFuzzy(const std::string& name);

    // Components
    template<typename T>
    std::vector<Attribute*>& getAllComponents() { return getAllComponents(rttr::type::get<T>()); }
    std::vector<Attribute*>& getAllComponents(rttr::type t);

    // Controllers
    template<typename T>
    T* getController();
    SceneController* getController(rttr::type t);
    template<typename T>
    bool hasController();
    bool hasController(rttr::type t);
    size_t controllerCount() const;
    SceneController* getController(size_t i);

    void startSession();
    void stopSession();

    void update();
    void debugDraw(DebugDraw& dd);

    void resetAttribute(Attribute* attrib);
    void _registerComponent(Attribute* c);
    void _unregisterComponent(Attribute* c);

    virtual void serialize(out_stream& out);
    virtual bool deserialize(in_stream& in, size_t sz);

    void write(out_stream& out);
    void read(in_stream& in);
    bool write(const std::string& fname);
    bool read(const std::string& fname);
private:
    SceneController* createController(rttr::type t);

    std::shared_ptr<GameObject>                             root_object;
    std::map<rttr::type, std::shared_ptr<SceneController>>  controllers;
    std::vector<SceneController*>                           updatable_controllers;
    std::map<
        rttr::type, 
        std::vector<Attribute*>
    >                                                       object_components;
};
STATIC_RUN(GameScene) {
    rttr::registration::class_<GameScene>("GameScene")
        .constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

template<typename T>
T* GameScene::getController() {
    return (T*)getController(rttr::type::get<T>());
}
template<typename T>
bool GameScene::hasController() {
    return hasController(rttr::type::get<T>());
}

#endif
