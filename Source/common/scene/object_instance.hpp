#ifndef OBJECT_INSTANCE_HPP
#define OBJECT_INSTANCE_HPP

#include "game_object.hpp"

class GameScene;
class ktObjectInstance : public GameObject {
    std::shared_ptr<GameScene> scene;
public:
    ktObjectInstance() {}
    ~ktObjectInstance() {}
    
    virtual OBJECT_TYPE                 getType() const { return OBJECT_INSTANCE; }
    
    void                                setScene(std::shared_ptr<GameScene> scene);
    std::shared_ptr<GameScene>          getScene() const;

    void                                onGui() override;
};

#endif
