#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/util/singleton.hpp"

template<typename T>
class AttribPool : public Singleton<AttribPool<T>> {
    std::vector<T> data;
    std::set<size_t> free_slots;
public:
    size_t acquire();
    void   free(size_t uid);
    T*     deref(size_t uid);
};

struct ecsArchetype {
    std::string name;
    ecsArchetype* parent;
    std::set<ecsArchetype*> children;
    std::vector<rttr::type> attributes;

    ecsArchetype& inherit(const char* name);
    ecsArchetype& require(const char* name);
    ecsArchetype& exclude(const char* name);
    ecsArchetype& optional(const char* name);
};

class ArchetypeLib : public Singleton<ArchetypeLib> {
    std::map<std::string, ecsArchetype*> archetypes_by_name;
    std::set<std::shared_ptr<ecsArchetype>> archetypes;
    std::map<std::string, ecsArchetype*> root_archetypes;
public:
    ecsArchetype* getArchetype(const char* name) {
        auto it = archetypes_by_name.find(name);
        if(it != archetypes_by_name.end()) {
            return it->second;
        }
        return defineArchetype(name);
    }

    ecsArchetype* defineArchetype(const char* name) {
        std::shared_ptr<ecsArchetype> ptr(new ecsArchetype());
        ptr->name = name;
        archetypes.insert(ptr);
        root_archetypes[name] = ptr.get();

        archetypes_by_name[name] = ptr.get();

        return ptr.get();
    }
    void defineInheritance(const char* child, const char* parent) {
        auto c = getArchetype(child);
        auto p = getArchetype(parent);
        p->children.insert(c);
        c->parent = p;
        root_archetypes.erase(child);
    }

    const std::map<std::string, ecsArchetype*>& getRootArchetypes() const {
        return root_archetypes;
    }
};

ecsArchetype& ecsArchetype::inherit(const char* name) {
    ArchetypeLib::get()->defineInheritance(this->name.c_str(), name);
    return *this;
}
ecsArchetype& ecsArchetype::require(const char* name) {
    return *this;
}
ecsArchetype& ecsArchetype::exclude(const char* name) {
    return *this;
}
ecsArchetype& ecsArchetype::optional(const char* name) {
    return *this;
}


struct ecsAttrib {
    virtual ~ecsAttrib() {}
    virtual void onGui() {}
};


struct ecsPosition : public ecsAttrib {
    gfxm::vec3 position;
    void onGui() override {

    }
};
struct ecsRotation : public ecsAttrib {
    gfxm::quat rotation;
    void onGui() override {

    }
};
struct ecsScale : public ecsAttrib {
    gfxm::vec3 scale;
    void onGui() override {

    }
};




static uint64_t nextEntityUid() {
    static uint64_t uid = 0;
    return uid++;
}

class ecsEntity {
    uint64_t uid;
    std::map<rttr::type, ecsAttrib*> attribs;
public:
    ecsEntity() {
        uid = nextEntityUid();
    }

    bool operator<(const ecsEntity& other) const {
        return uid < other.uid;
    }

    size_t attribCount() const {
        return attribs.count();
    }
    ecsAttrib* getAttrib(size_t index) {
        auto it = attribs.begin();
        std::advance(it, index);
        return it->second;
    }
};

class ecsWorld {
    std::set<ecsEntity> entities;
public:
    ecsEntity createEntity() {
        ecsEntity ent;
        entities.insert(ent);
        return ent;
    }

    void onGui() {
        for(auto e : entities) {
            ImGui::Selectable("Entity");
        }
    }
};

void defineAttrib(const char* name) {

}

ecsArchetype& defineArchetype(const char* name) {
    return *ArchetypeLib::get()->defineArchetype(name);
}

class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
public:
    DocEcsWorld() {
        defineAttrib("Transform");
        defineAttrib("LightIntensity");
        defineAttrib("LightColor");

        defineArchetype("Node3D")
            .require("Transform");
        defineArchetype("OmniLight")
            .inherit("Node3D")
            .require("LightIntensity")
            .require("LightColor");
        defineArchetype("Model")
            .inherit("Node3D")
            .require("Mesh")
            .require("Materials");
        defineArchetype("Lightbulb")
            .inherit("OmniLight")
            .inherit("Model");
    }

    void onGuiArchetype(ecsArchetype* arch) {
        if(arch->children.empty()) {
            ImGui::TreeAdvanceToLabelPos();
            if(ImGui::Selectable(arch->name.c_str())) {

            }
        } else {
            bool node_open = ImGui::TreeNodeEx(
                (void*)arch,
                0,
                arch->name.c_str()
            );
            if(node_open) {
                for(auto& a : arch->children) {
                    onGuiArchetype(a);
                }
                ImGui::TreePop();
            }
        }
    }

    void onGui(Editor* ed, float dt) override {
        for(auto& it : ArchetypeLib::get()->getRootArchetypes()) {
            auto arch = it.second;
            onGuiArchetype(arch);            
        }
    }
    void onGuiToolbox(Editor* ed) override {
        if(ImGui::Button("Create entity")) {
            world.createEntity();
        }
        world.onGui();
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
