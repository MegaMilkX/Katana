#ifndef DOC_ECS_WORLD_HPP
#define DOC_ECS_WORLD_HPP

#include "editor_document.hpp"
#include "../common/resource/ecs_world.hpp"

#include "../common/util/singleton.hpp"

#include <assert.h>

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

class bitset {
    std::vector<char> bytes;
public:
    size_t bitCount() const {
        const int bits_per_byte = 8;
        return bytes.size() * bits_per_byte;
    }
    size_t enabledBitCount() const {
        size_t count = 0;
        for(size_t i = 0; i < bitCount(); ++i) {
            if(test(i)) {
                ++count;
            }
        }
        return count;//attribs.count();
    }

    void clear() {
        bytes.clear();
    }

    void resize(size_t sz) {
        bytes.resize(sz);
    }

    void set(size_t bit, bool value) {
        const int bits_per_byte = 8;
        int byte_id = bit / bits_per_byte;
        bit = bit - byte_id * bits_per_byte;
        if(bytes.size() <= byte_id) {
            resize(byte_id + 1);
        }
        bytes[byte_id] |= (1 << bit);
    }

    bool test(size_t bit) const {
        const int bits_per_byte = 8;
        int byte_id = bit / bits_per_byte;
        bit = bit - byte_id * bits_per_byte;
        if(bytes.size() <= byte_id) {
            return false;
        }
        return bytes[byte_id] & (1 << bit);
    }
};

class dbAttrib : public Singleton<dbAttrib> {
    std::map<std::string, int> name_to_index;
    std::map<int, std::string> index_to_name;
public:
    void reg(const char* name) {
        size_t index = name_to_index.size();
        name_to_index[name] = index;
        index_to_name[index] = name;
    }
    int getId(const char* name) const {
        auto it = name_to_index.find(name);
        if(it == name_to_index.end()) {
            return -1;
        }
        return it->second;
    }
    const std::string& getName(size_t id) const {
        auto it = index_to_name.find(id);
        if(it == index_to_name.end()) {
            return "ERROR_ATTRIB_ID";
        }
        return it->second;
    }
    size_t count() const {
        return name_to_index.size();
    }
};

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

    void getAttrib(const char* name) {
        int id = dbAttrib::get()->getId(name);
        assert(id >= 0);
        attrib_bits.set(id, true);
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
    dbAttrib::get()->reg(name);
}

ecsArchetype& defineArchetype(const char* name) {
    return *ArchetypeLib::get()->defineArchetype(name);
}

class DocEcsWorld : public EditorDocumentTyped<EcsWorld> {
    ecsWorld world;
public:
    DocEcsWorld() {
        defineAttrib("Name");
        defineAttrib("Translation");
        defineAttrib("Rotation");
        defineAttrib("Scale");
        defineAttrib("LightIntensity");
        defineAttrib("LightColor");

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

        ecsEntity entt;
        entt.getAttrib("Translation");
        entt.getAttrib("LightColor");
        entt.getAttrib("Name");
        for(size_t i = 0; i < entt.getAttribBits().bitCount(); ++i) {
            if(entt.getAttribBits().test(i)) {
                std::cout << 1;
                auto name = dbAttrib::get()->getName(i);
                ImGui::Text(name.c_str());
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
