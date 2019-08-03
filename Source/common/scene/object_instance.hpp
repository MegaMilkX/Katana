#ifndef OBJECT_INSTANCE_HPP
#define OBJECT_INSTANCE_HPP

#include "game_object.hpp"

class GameScene;
class ktObjectInstance : public GameObject {
    std::shared_ptr<GameScene> scene;
    std::map<GameObject*, GameObject*> original_to_instance;
public:
    ktObjectInstance() {}
    ~ktObjectInstance() {}
    
    virtual OBJECT_TYPE                 getType() const { return OBJECT_INSTANCE; }
    
    void                                setScene(std::shared_ptr<GameScene> scene);
    GameScene*                          getScene() const;

    void                                _createInstancedNode(GameObject* o);
    void                                _removeInstancedNode(GameObject* o);
    void                                _createInstancedAttrib(Attribute* a);
    void                                _removeInstancedAttrib(Attribute* a);
};

#endif
