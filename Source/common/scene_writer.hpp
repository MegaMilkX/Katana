#ifndef SCENE_WRITER_HPP
#define SCENE_WRITER_HPP

#include "resource/resource.h"
#include "scene/game_object.hpp"

class ktISceneWriter {
public:
    virtual ~ktISceneWriter() {}

    // Write resource as a reference by string name, or if its name is "" - embed the resource into the scene
    void write(Resource* res);
    // Write a string
    void write(const std::string& str);
    // Write just a plain value
    template<typename T>
    void write(const T& v);
};

class ktSceneWriter : public ktISceneWriter {
public:
    void push(GameObject* o);
    void push(Attribute* attrib);
    void pop();
};

#endif
