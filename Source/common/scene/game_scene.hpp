#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include "../resource/resource.h"

#include "node.hpp"
#include "object_instance.hpp"

#include "../attributes/camera.hpp"

#include "../../common/debug_draw.hpp"

#include "scene_controller.hpp"
#include "../serializable.hpp"

class GameScene : public ktNode, public Resource {
    RTTR_ENABLE(Resource)
public:
    GameScene();
    ~GameScene();

    void clear();

    std::vector<ktNode*> findObjectsFuzzy(const std::string& name);

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

    void addListener(SceneController* lis);
    void removeListener(SceneController* lis);

    void startSession();
    void stopSession();

    void update();
    void debugDraw(DebugDraw& dd);

    virtual void serialize(out_stream& out);
    virtual bool deserialize(in_stream& in, size_t sz);

    virtual const char* getWriteExtension() const { return "so"; }
private:
    SceneController* createController(rttr::type t);

    virtual void                        _registerComponent(Attribute* attrib);
    virtual void                        _unregisterComponent(Attribute* attrib);
    virtual void                        _readdComponent(Attribute* attrib);

    std::set<SceneController*>                              foreign_listeners;
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
