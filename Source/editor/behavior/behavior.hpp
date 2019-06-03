#ifndef BEHAVIOR_HPP
#define BEHAVIOR_HPP

#include "../scene/game_object.hpp"
#include "../reflectable.hpp"
#include "../serializable.hpp"

class GameScene;
class Behavior : public Reflectable, public Serializable {
    RTTR_ENABLE()
    friend GameScene;
public:
	virtual ~Behavior() {}
	
	virtual void onInit() {}
	virtual void onStart() {}
	virtual void onUpdate() {}
	virtual void onCleanup() {}
	
	virtual void onGui() {}

	GameObject* getOwner() { return object; }
private:
	GameObject* object;
};

#endif
