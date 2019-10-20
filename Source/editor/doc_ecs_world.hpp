#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/util/singleton.hpp"

#include <assert.h>

#include "../common/ecs/bytepool.hpp"

#include "../common/util/attrib_type.hpp"

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

struct ecsName : public ecsAttrib {
    std::string name;
};
struct ecsTranslation : public ecsAttrib {
    gfxm::vec3 position;
};
struct ecsRotation : public ecsAttrib {
    gfxm::quat rotation;
};
struct ecsScale : public ecsAttrib {
    gfxm::vec3 scale;
};
struct ecsLightIntensity : public ecsAttrib {
    float intensity;
};
struct ecsLightColor : public ecsAttrib {
    gfxm::vec3 color;
};




static uint64_t nextEntityUid() {
    static uint64_t uid = 0;
    return uid++;
}

#include "../common/util/bitset.hpp"





class ecsEntity {
    uint64_t uid;
    bitset attrib_bits;
public:
    ecsEntity() {
        uid = nextEntityUid();
    }

    bool operator<(const ecsEntity& other) const {
        return uid < other.uid;
    }

    const bitset& getAttribBits() const {
        return attrib_bits;
    }

    template<typename T>
    void setAttrib(const T& attrib) {
        attrib_bits.set(get_attrib_id<T>(), true);
    }
};


class ecsAttribArrayBase {
    std::map<ecsEntity, size_t> ent_to_attrib;
public:
    virtual ~ecsAttribArrayBase() {}
    virtual size_t getInstanceId(ecsEntity ent) {
        auto it = ent_to_attrib.find(ent);
        if(it == ent_to_attrib.end()) {
            return std::numeric_limits<size_t>::max();
        }
        return it->second;
    }
};

template<typename T>
class ecsAttribArray : public ecsAttribArrayBase {
    std::vector<T> data;
    std::set<size_t> free_slots;
public:
    size_t acquire();
    void   free(size_t uid);
    T*     deref(size_t uid);
};

class ecsAttribData {
    std::unordered_map<
        attrib_id, 
        std::shared_ptr<ecsAttribArrayBase>
    > attrib_arrays;
public:
    size_t getAttribInstanceId(ecsEntity ent, attrib_id attrib) {
        auto attrib_array = attrib_arrays[attrib];
        return attrib_array->getInstanceId(ent);
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

template<typename T>
void defineAttrib(const char* name) {
    get_attrib_type_storage().set_name(get_attrib_id<T>(), name);
}

ecsArchetype& defineArchetype(const char* name) {
    return *ArchetypeLib::get()->defineArchetype(name);
}

template<typename... Args>
class ecsArch {

};

class ecsTest : public ecsArch<ecsTranslation, ecsRotation, ecsScale, ecsName, ecsLightIntensity, ecsLightColor> {
public:

};

template<typename... Args>
class ecsSystem {
public:
    
};

class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
public:
    DocEcsWorld() {
        defineAttrib<ecsName>("Name");
        defineAttrib<ecsTranslation>("Translation");
        defineAttrib<ecsRotation>("Rotation");
        defineAttrib<ecsScale>("Scale");
        defineAttrib<ecsLightIntensity>("LightIntensity");
        defineAttrib<ecsLightColor>("LightColor");

        defineArchetype("Node3D")
            .require("Translation");
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
        /*
        if(ImGui::Button("Create entity")) {
            world.createEntity();
        }
        world.onGui();*/

        rttr::type::get<int>();
        rttr::type::get_by_name("ijiji");

        ecsEntity entt;
        entt.setAttrib(ecsName());
        entt.setAttrib(ecsTranslation());
        for(size_t i = 0; i < entt.getAttribBits().bitCount(); ++i) {
            if(entt.getAttribBits().test(i)) {
                std::cout << 1;
                auto name = get_attrib_type_storage().get_name(i);
                ImGui::Text(name);
            } else {
                std::cout << 0;
            }
        }
        std::cout << std::endl;
    }
};
STATIC_RUN(DocEcsWorld) {
    regEditorDocument<DocEcsWorld>({ "ecsw" });
}

#endif
