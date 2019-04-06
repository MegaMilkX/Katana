#include "scene.hpp"

Scene3::~Scene3() {
    clear();
}

void Scene3::clear() {
    for(auto& kv : attribs) {
        auto hmgr = get_handle_mgr(kv.first);
        if(hmgr) {
            for(auto h : kv.second) {
                hmgr->free(h);
            }
        }
    }
    attribs.clear();
    handle_to_local_attrib_id.clear();
}

ghandle<SceneObject> Scene3::createObject() {
    return createObject("Unnamed");
}
ghandle<SceneObject> Scene3::createObject(const std::string& name) {
    ghandle<SceneObject> hdl = get_handle_mgr<SceneObject>().acquire();
    if(hdl) {
        hdl->scene = this;
        objects.emplace_back(hdl);

        new(hdl.deref())(SceneObject);
        hdl->setName(name);
    } else {
        LOG_WARN("Failed to create scene object");
    }
    return hdl;
}

handle_ Scene3::createAttrib(rttr::type t) { 
    auto hmgr = get_handle_mgr(t);
    handle_ hdl = 0;
    if(hmgr) {
        hdl = hmgr->acquire();
        if(hdl) {
            attribs[t].emplace_back(hdl);
            handle_to_local_attrib_id[t][hdl] = attribs[t].size() - 1;
        } else {
            LOG_WARN("Failed to create attrib of type " << t.get_name().to_string());
        }
    } else {
        LOG_WARN("Failed to create attrib: handle manager for type " << t.get_name().to_string() << " doesn't exist");
    }
    return hdl;
}

std::vector<handle_> Scene3::getAttribs(rttr::type t) {
    return attribs[t];
}

void Scene3::logStats() {
    LOG("==== Scene3 stats ====================");

    LOG("Object count: " << objects.size());
    for(auto o : objects) {
        LOG(o->getName());
    }
    LOG("Attrib type count: " << attribs.size());
    for(auto kv : attribs) {
        LOG(kv.first.get_name().to_string() << " count: " << kv.second.size());
    }

    LOG("==== Scene3 stats end ================");
}