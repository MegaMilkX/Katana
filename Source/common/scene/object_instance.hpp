#ifndef OBJECT_INSTANCE_HPP
#define OBJECT_INSTANCE_HPP

#include "node.hpp"

class GameScene;
class ktObjectInstance : public ktNode {
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
