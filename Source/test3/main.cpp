#include <vector>
#include <map>
#include <rttr/type>

#include "../common/util/log.hpp"

#include "../common/resource/mesh.hpp"
#include "../common/resource/material.hpp"

#include "scene_object.hpp"

class TestAttrib : public Attribute {
public:
	std::string val;
};
STATIC_RUN(TestAttrib) {
	rttr::registration::class_<TestAttrib>("TestAttrib")
		.constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

class TestBhvr : public Behavior {
public:
	void onUpdate() {
		std::cout << this << ": " << getOwner()->getAttrib<TestAttrib>()->val << std::endl;
	}
};
STATIC_RUN(TestBhvr) {
	rttr::registration::class_<TestBhvr>("TestBhvr")
		.constructor<>()(
            rttr::policy::ctor::as_raw_ptr
        );
}

struct Attrib {
	Attrib* prev = 0;
	Attrib* next = 0;

	void createAttrib() {
		if(!next) {
			next = new Attrib();
		} else {
			next->createAttrib();
		}
	}
};

struct Object {
	std::string name;
	
};

#include "fbx_import.hpp"

int main() {
	SceneObject so;
	so.setBehavior<TestBhvr>();
	so.getAttrib<TestAttrib>()->val = "hello";

	std::cout << so.getAttrib<TestAttrib>()->val << std::endl;

	for(int i = 0; i < 10; ++i) {
		so.getBehavior()->onUpdate();
	}
	
	std::getchar();
	
	//importFbx("test.fbx", &so);

    return 0;
}
