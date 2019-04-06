#ifndef SCENE_HPP
#define SCENE_HPP

#include <memory>
#include "../../common/util/log.hpp"
#include "../util/reg_type.hpp"

#include "scene_object.hpp"

class Scene3 : public std::enable_shared_from_this<Scene3> {
public:
    static std::shared_ptr<Scene3> create() {
        return std::shared_ptr<Scene3>(new Scene3());
    }
    ~Scene3();

    void                        clear();

    ghandle<SceneObject>        createObject();
    ghandle<SceneObject>        createObject(const std::string& name);

    template<typename T>
    ghandle<T>                  createAttrib();
    handle_                     createAttrib(rttr::type);
    template<typename T>
    std::vector<ghandle<T>>     getAttribs();
    std::vector<handle_>        getAttribs(rttr::type);

    void                        logStats();
private:
    Scene3() {}

    std::vector<ghandle<SceneObject>>   objects;
    std::map<
        rttr::type, 
        std::vector<handle_>
    >                                   attribs;
    std::map<
        rttr::type,
        std::map<handle_, uint64_t>
    >                                   handle_to_local_attrib_id;
};

template<typename T>
ghandle<T> Scene3::createAttrib() {
    return createAttrib(rttr::type::get<T>());
}

template<typename T>
std::vector<ghandle<T>> Scene3::getAttribs() {
    return *(std::vector<ghandle<T>>*)&getAttribs(rttr::type::get<T>());
}

#endif
