#ifndef SCENE_OBJECT_HPP
#define SCENE_OBJECT_HPP

#include <map>
#include <set>
#include <rttr/type>
#include <rttr/registration>
#include "../common/util/static_run.h"

struct SceneObject;

class Attribute {
public:
	virtual ~Attribute() {}
	
	virtual void onGui() {}
};

class Behavior {
	friend SceneObject;
public:
	virtual ~Behavior() {}
	
	virtual void onInit() {}
	virtual void onStart() {}
	virtual void onUpdate() {}
	virtual void onCleanup() {}
	
	virtual void onGui() {}

	SceneObject* getOwner() { return object; }
private:
	SceneObject* object;
};

struct SceneObject {
	~SceneObject();

	template<typename T>
	void 					setBehavior();
	void 					setBehavior(rttr::type t);
	Behavior* 				getBehavior() const;
	void					eraseBehavior();

	template<typename T>
	T* 						getAttrib();
	template<typename T>
	T*						findAttrib();
	Attribute* 				getAttrib(rttr::type t);
	Attribute*				findAttrib(rttr::type t);
	void					eraseAttrib(rttr::type t);
};

template<typename T>
void SceneObject::setBehavior() {
	setBehavior(rttr::type::get<T>());
}
template<typename T>
T* SceneObject::getAttrib() {
	return (T*)getAttrib(rttr::type::get<T>());
}
template<typename T>
T* SceneObject::findAttrib() {
	return (T*)findAttrib(rttr::type::get<T>());
}

#endif
