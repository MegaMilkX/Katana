#include "node.hpp"

#include <stack>

#define MINIZ_HEADER_FILE_ONLY
#include "../../common/lib/miniz.c"

#include "../lib/imguizmo/ImGuizmo.h"

#include "game_scene.hpp"
#include "object_instance.hpp"

ktNode::ktNode() {

}
ktNode::~ktNode() {
    // Tell parent transform that this one doesn't exist anymore
    transform.setParent(0);

    deleteAllComponents();

    for(auto o : children) {
        delete o;
    }
}

void ktNode::setEnabled(bool v) { 
    _enabled = v;
    for(auto c : children) {
        c->setEnabled(v);
    }
}
bool ktNode::isEnabled() const { 
    return _enabled; 
}

void ktNode::copy(ktNode* other, OBJECT_FLAGS f) {
    for(auto o : children) {
        delete o;
    }
    children.clear();
    deleteAllComponents();
    components.clear();

    std::vector<std::pair<ktNode*, ktNode*>> target_source_pairs;

    std::function<void(ktNode*, ktNode*)> copy_tree_fn;
    copy_tree_fn = [this, &copy_tree_fn, &target_source_pairs, &f](ktNode* target, ktNode* source){
        target->name = source->name;
        if(target != this) {
            target->getTransform()->setTransform(source->getTransform()->getLocalTransform());
        } else {
            //target->getTransform()->setScale(source->getTransform()->getScale());
        }
        target->_enabled = source->_enabled;
        for(auto c : source->children) {
            if(c->getType() == OBJECT_INSTANCE) {
                auto inst = target->createInstance(((ktObjectInstance*)c)->getScene());
                inst->getTransform()->setTransform(c->getTransform()->getLocalTransform());
            } else {
                copy_tree_fn(target->createChild(f), c);
            }
        }
        target_source_pairs.emplace_back(std::make_pair(target, source));
    };

    copy_tree_fn(this, other);

    for(auto pair : target_source_pairs) {
        for(auto& kv : pair.second->components) {            
            dstream strm;
            kv.second->serialize(strm);
            strm.jump(0);
            auto attr = pair.first->createComponent(kv.first);

            auto parent_cached = this->parent;
            this->parent = 0; // Tricking components into thinking that this is the root node,
                        // which it is for purposes of deserialization
            attr->deserialize(strm, strm.bytes_available());
            this->parent = parent_cached;
        }
    }
}

void ktNode::setName(const std::string& name) { 
    this->name = name; 
}
const std::string& ktNode::getName() const { 
    return name; 
}

ktNode* ktNode::getRoot() {
    if(!parent) {
        return this;
    } else {
        return parent->getRoot();
    }
}
ktNode* ktNode::getParent() {
    return parent;
}

TransformNode* ktNode::getTransform() {
    return &transform;
}

ktNode* ktNode::createChild(OBJECT_FLAGS f) {
    ktNode* o = new ktNode();
    o->parent = this;
    o->getTransform()->setParent(&transform);
    o->_flags = f;
    children.insert(o);
    return o;
}

ktObjectInstance* ktNode::createInstance(std::shared_ptr<GameScene> scn) {
    ktObjectInstance* o = new ktObjectInstance();
    o->parent = this;
    o->getTransform()->setParent(&transform);
    children.insert(o);

    o->setScene(scn);
    
    return o;
}

size_t ktNode::childCount() {
    return children.size();
}
ktNode* ktNode::getChild(size_t i) {
    auto it = children.begin();
    std::advance(it, i);
    return (*it);
}
ktNode* ktNode::getChild(const std::string& name) {
    for(auto o : children) {
        if(o->getName() == name) {
            return o;
        }
    }
    return 0;
}
ktNode* ktNode::findObject(const std::string& name) {
    for(auto o : children) {
        if(o->getName() == name) return o;
        ktNode* r = o->findObject(name);
        if(r) return r;
    }
    return 0;
}
void ktNode::getAllObjects(std::vector<ktNode*>& result) {
    for(auto o : children) {
        o->getAllObjects(result);
    }
    result.emplace_back(this);
}

void ktNode::deleteChild(ktNode* child) {
    auto it = children.find(child);
    if(it == children.end()) {
        return;
    }
    delete child;
    children.erase(child);
}

std::shared_ptr<Attribute> ktNode::find(rttr::type component_type) {
    if(components.count(component_type) == 0) {
        return std::shared_ptr<Attribute>();
    }
    return components[component_type];
}
std::shared_ptr<Attribute> ktNode::get(rttr::type component_type) {
    auto c = find(component_type);
    if(!c) {
        return createComponent(component_type);
    }
    return c;
}
size_t ktNode::componentCount() {
    return components.size();
}
std::shared_ptr<Attribute> ktNode::getById(size_t id) {
    auto it = components.begin();
    std::advance(it, id);
    return it->second;
}
void ktNode::deleteComponentById(size_t id) {
    auto it = components.begin();
    std::advance(it, id);
    _unregisterComponent(it->second.get());
    components.erase(it);
}
void ktNode::deleteAllComponents() {
    for(auto& kv : components) {
        _unregisterComponent(kv.second.get());
    }
    components.clear();
}

void ktNode::refreshAabb() {
    for(size_t i = 0; i < childCount(); ++i) {
        getChild(i)->refreshAabb();
    }

    aabb = gfxm::aabb(
        transform.getWorldPosition(),
        transform.getWorldPosition()
    );
    bool has_aabb = false;
    for(auto& kv : components) {
        auto oc = kv.second;
        gfxm::aabb box;
        if(oc->buildAabb(box)) {
            if(!has_aabb) {
                aabb = box;
                has_aabb = true;
            } else {
                gfxm::expand_aabb(aabb, box.from);
                gfxm::expand_aabb(aabb, box.to);
            }
        }
    }
    if(has_aabb) {
        gfxm::vec3 cube_pts[8] = {
            { aabb.from.x, aabb.from.y, aabb.from.z },
            { aabb.to.x, aabb.from.y, aabb.from.z },
            { aabb.to.x, aabb.to.y, aabb.from.z },
            { aabb.from.x, aabb.to.y, aabb.from.z },
            { aabb.from.x, aabb.from.y, aabb.to.z },
            { aabb.to.x, aabb.from.y, aabb.to.z },
            { aabb.to.x, aabb.to.y, aabb.to.z },
            { aabb.from.x, aabb.to.y, aabb.to.z }
        };
        for(size_t i = 0; i < 8; ++i) {
            cube_pts[i] = transform.getWorldTransform() * gfxm::vec4(cube_pts[i], 1.0f);
        }
        aabb = gfxm::aabb(
            cube_pts[0],
            cube_pts[0]
        );
        for(size_t i = 0; i < 8; ++i) {
            gfxm::expand_aabb(aabb, cube_pts[i]);
        }
    }

    if(!has_aabb && childCount()) {
        auto c = getChild(0);
        aabb = gfxm::aabb(
            c->getAabb().from,
            c->getAabb().to
        );
    }

    for(size_t i = 0; i < childCount(); ++i) {
        auto& child_box = getChild(i)->getAabb();
        // TODO: Ignore empty aabbs
        gfxm::expand_aabb(aabb, child_box.from);
        gfxm::expand_aabb(aabb, child_box.to);
    }
}
void ktNode::setAabb(const gfxm::aabb& box) {
    aabb = box;
}
const gfxm::aabb& ktNode::getAabb() const {
    return aabb;
}

static bool zipAdd(void* data, size_t sz, mz_zip_archive& archive, const std::string& name) {    
    if(!mz_zip_writer_add_mem(
        &archive, 
        name.c_str(), 
        data, 
        sz, 
        0
    )){
        LOG_ERR("Failed to mz_zip_writer_add_mem() ");
    }
    return true;
}

static bool zipAddFromStream(in_stream& in, mz_zip_archive& archive, const std::string& name) {
    std::vector<char> buf;
    buf.resize(in.bytes_available());
    in.read((char*)buf.data(), in.bytes_available());
    
    if(!mz_zip_writer_add_mem(
        &archive, 
        name.c_str(), 
        buf.data(), 
        buf.size(), 
        0
    )){
        LOG_ERR("Failed to mz_zip_writer_add_mem() ");
    }
    return true;
}

enum TRANSFORM_GIZMO_MODE {
    TGIZMO_T,
    TGIZMO_R,
    TGIZMO_S
};

enum TRANSFORM_GIZMO_SPACE {
    TGIZMO_LOCAL,
    TGIZMO_WORLD
};
static TRANSFORM_GIZMO_MODE sGizmoMode;
static TRANSFORM_GIZMO_SPACE sGizmoSpace;

void ktNode::onGui() {
    static TransformNode* last_transform = 0;
    static int t_sync = -1;
    static gfxm::vec3 euler;

    if(ImGui::Checkbox("Enabled", &_enabled)) {
        setEnabled(_enabled);
    }
    char buf[256];
    memset(buf, 0, 256);
    memcpy(buf, getName().c_str(), getName().size());
    if(ImGui::InputText("Name", buf, 256)) {
        setName(buf);
    }

    if(getParent() != 0) {
        if(last_transform != getTransform() || t_sync != getTransform()->getSyncId()) {
            last_transform = getTransform();
            t_sync = getTransform()->getSyncId();
            euler = getTransform()->getEulerAngles();
        }

        auto t = getTransform()->getPosition();
        auto r = getTransform()->getEulerAngles();
        auto s = getTransform()->getScale();

        ImGui::Separator();
        if(ImGui::DragFloat3("Translation", (float*)&t, 0.001f)) {
            getTransform()->setPosition(t);
            getRoot()->refreshAabb();
            t_sync = getTransform()->getSyncId();
        }
        if(ImGui::DragFloat3("Rotation", (float*)&euler, 0.001f)) {
            getTransform()->setRotation(euler);
            getRoot()->refreshAabb();
            t_sync = getTransform()->getSyncId();
        }
        if(ImGui::DragFloat3("Scale", (float*)&s, 0.001f)) {
            getTransform()->setScale(s);
            getRoot()->refreshAabb();
            t_sync = getTransform()->getSyncId();
        }

        ImGui::Separator();
        ImGui::Text("Gizmo:");
        int gizmo_mode = (int)sGizmoMode;
        int gizmo_space = (int)sGizmoSpace;
        ImGui::RadioButton("T", &gizmo_mode, TGIZMO_T);
        ImGui::SameLine(); ImGui::RadioButton("Local", &gizmo_space, TGIZMO_LOCAL);
        ImGui::RadioButton("R", &gizmo_mode, TGIZMO_R);
        ImGui::SameLine(); ImGui::RadioButton("World", &gizmo_space, TGIZMO_WORLD);
        ImGui::RadioButton("S", &gizmo_mode, TGIZMO_S);
        sGizmoMode = (TRANSFORM_GIZMO_MODE)gizmo_mode;
        sGizmoSpace = (TRANSFORM_GIZMO_SPACE)gizmo_space;
    }
}

void ktNode::onGizmo(GuiViewport& vp) {
    if(getParent() != 0) {
        auto vp_pos = vp.getPos();
        auto vp_size = vp.getSize();
        //ImGuizmo::SetRect(vp_pos.x, vp_pos.y, vp_size.x, vp_size.y);

        ImVec2 sz;
        sz.x = vp.getSize().x;
        sz.y = vp.getSize().y;
        gfxm::mat4 proj = vp.getProjection();
        gfxm::mat4 view = vp.getView();
/*
        ImGuizmo::SetRect(
            ImVec2(ImGui::GetCursorScreenPos()).x, 
            ImVec2(ImGui::GetCursorScreenPos()).y, 
            sz.x, 
            sz.y
        ); */
        gfxm::mat4 model = getTransform()->getWorldTransform();
        gfxm::mat4 dModel(1.0f);

        ImGuizmo::MODE space_mode = ImGuizmo::MODE::LOCAL;
        if(sGizmoSpace == TGIZMO_WORLD) {
            space_mode = ImGuizmo::MODE::WORLD;
        }
        
        if(sGizmoMode == TGIZMO_T) {
            ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::TRANSLATE, space_mode, (float*)&model, (float*)&dModel, 0);
            if(ImGuizmo::IsUsing()){
                gfxm::vec4 dT = dModel[3];
                getTransform()->translate(
                    gfxm::inverse(getTransform()->getParentTransform()) * dT
                );
                getRoot()->refreshAabb();
            }
        } else if(sGizmoMode == TGIZMO_R) {
            ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::ROTATE, space_mode, (float*)&model, (float*)&dModel, 0);
            if(ImGuizmo::IsUsing()){
                gfxm::quat q = gfxm::to_quat(gfxm::to_mat3(dModel));
                getTransform()->rotate(q);
                getRoot()->refreshAabb();
            }
        } else if(sGizmoMode == TGIZMO_S) {
            ImGuizmo::Manipulate((float*)&view, (float*)&proj, ImGuizmo::OPERATION::SCALE, space_mode, (float*)&model, (float*)&dModel, 0);
            if(ImGuizmo::IsUsing()){
                gfxm::mat4 m4 = model;
                m4 = gfxm::inverse(getTransform()->getParentTransform()) * m4;
                gfxm::vec3 dscl(
                    gfxm::length(m4[0]),
                    gfxm::length(m4[1]),
                    gfxm::length(m4[2])
                );

                getTransform()->setScale(dscl);
                getRoot()->refreshAabb();
            }
        }
    }
    
    for(size_t i = 0; i < componentCount(); ++i) {
        getById(i)->onGizmo(vp);
    }
}

// ==== Private ====================================

std::shared_ptr<Attribute> ktNode::createComponent(rttr::type t) {
    if(!t.is_valid()) {
        LOG_WARN(t.get_name().to_string() << " - not a valid type");
        return std::shared_ptr<Attribute>();
    }
    rttr::variant v = t.create();
    if(!v.get_type().is_pointer()) {
        LOG_WARN(t.get_name().to_string() << " - rttr constructor policy must be pointer");
        return std::shared_ptr<Attribute>();
    }
    Attribute* c = v.get_value<Attribute*>();
    if(!c) {
        LOG_WARN("Failed to create component " << t.get_name().to_string());
        return std::shared_ptr<Attribute>();
    }
    c->owner = this;
    auto ptr = std::shared_ptr<Attribute>(c);

    components[t] = ptr;
    ptr->onCreate();
    _registerComponent(c);
    return ptr;
}

bool ktNode::serializeComponents(std::ostream& out) {
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_writer_init_heap(&archive, 0, 65537)) {
        LOG("Failed to create archive file in memory");
        return false;
    }

    for(auto& kv : components) {
        auto& c = kv.second;
        dstream strm;
        if(c->serialize(strm)) {
            strm.jump(0);
            zipAddFromStream(strm, archive, c->get_type().get_name().to_string());
        }
    }

    void* archbuf = 0;
    size_t sz = 0;
    mz_zip_writer_finalize_heap_archive(&archive, &archbuf, &sz);
    std::vector<char> buf = std::vector<char>((char*)archbuf, (char*)archbuf + sz);
    mz_zip_writer_end(&archive);

    out.write(buf.data(), buf.size());
    return true;
}

void ktNode::write(out_stream& out) {
    ktNode* obj = this;

    std::map<ktNode*, uint64_t> oid_map;
    std::vector<ktNode*> objects;
    std::map<rttr::type, std::vector<Attribute*>> attribs;

    std::stack<ktNode*> stack;
    ktNode* current = obj;
    while(current != 0 || (!stack.empty())) {
        if(current == 0) {
            current = stack.top();
            stack.pop();
        }

        if(current->getFlags() == OBJECT_FLAG_TRANSIENT) {
            current = 0;
            continue;
        }

        uint64_t id = oid_map.size();
        oid_map[current] = id;
        objects.emplace_back(current);

        for(size_t i = 0; i < current->childCount(); ++i) {
            stack.push(current->getChild(i));
        }

        for(size_t i = 0; i < current->componentCount(); ++i) {
            auto attrib = current->getById(i);
            attribs[attrib->get_type()].emplace_back(attrib.get());
        }

        current = 0;
    }

    DataWriter dw(&out);
    dw.write<uint64_t>(objects.size());
    for(size_t i = 0; i < objects.size(); ++i) {
        auto o = objects[i];
        if(o->getParent()) {
            dw.write<uint64_t>(oid_map[o->getParent()]);
        } else {
            dw.write<uint64_t>(-1);
        }
        dw.write(o->getName());
        dw.write(o->getTransform()->getPosition());
        dw.write(o->getTransform()->getRotation());
        dw.write(o->getTransform()->getScale());
        dw.write<int8_t>(o->getType());
        if(o->getType() == OBJECT_INSTANCE) {
            auto scn = ((ktObjectInstance*)o)->getScene();
            if(scn) {
                dw.write(scn->Name()); // Name() is a resource name, getName() is a node name TODO: FIX IT
            } else {
                dw.write(std::string());
            }
        }
    }

    dw.write<uint32_t>(attribs.size());
    for(auto kv : attribs) {
        rttr::type t = kv.first;
        auto& vec = kv.second;
        dw.write(t.get_name().to_string());
        dw.write<uint64_t>(vec.size());
        for(size_t i = 0; i < vec.size(); ++i) {
            auto a = vec[i];
            dw.write<uint64_t>(oid_map[a->getOwner()]);
            
            dstream sdata;
            a->serialize(sdata);
            dw.write<uint64_t>(sdata.size()); // bytes written
            dw.write(sdata.getBuffer());
        }
    }
}
void ktNode::read(in_stream& in) {
    std::vector<ktNode*> objects;
    objects.emplace_back(this);
    
    DataReader dr(&in);
    uint64_t object_count = dr.read<uint64_t>();
    objects.resize(object_count);/*
    for(uint64_t i = 1; i < object_count; ++i) {
        objects.emplace_back(new ktNode());
    } */

    if(object_count == 0) {
        LOG_WARN("No objects in an .so data")
        return;
    }

    {
        // TODO: Problem: Root can't be an instance?
        ktNode* o = 0;
        uint64_t p = dr.read<uint64_t>();
        std::string name = dr.readStr();
        gfxm::vec3 t = dr.read<gfxm::vec3>();
        gfxm::quat r = dr.read<gfxm::quat>();
        gfxm::vec3 s = dr.read<gfxm::vec3>();
        OBJECT_TYPE type  = (OBJECT_TYPE)dr.read<int8_t>();
        if(type == OBJECT_INSTANCE) {
            std::string ref_name = dr.readStr();
            //((ktObjectInstance*)o)->setScene(retrieve<GameScene>(ref_name));
        } else {
            //o = new ktNode();
        }
        o = objects[0];
        o->setName(name);
        o->getTransform()->setPosition(t);
        o->getTransform()->setRotation(r);
        o->getTransform()->setScale(s);
    }

    for(uint64_t i = 1; i < object_count; ++i) {
        ktNode* o = 0;

        uint64_t p = dr.read<uint64_t>();
        std::string name = dr.readStr();
        gfxm::vec3 t = dr.read<gfxm::vec3>();
        gfxm::quat r = dr.read<gfxm::quat>();
        gfxm::vec3 s = dr.read<gfxm::vec3>();
        OBJECT_TYPE type  = (OBJECT_TYPE)dr.read<int8_t>();
        std::string ref_name;
        if(type == OBJECT_INSTANCE) {
            ref_name = dr.readStr();
            o = new ktObjectInstance();
        } else {
            o = new ktNode();
        }
        
        objects[i] = o;
        o->parent = objects[p]; // OK: Thanks to order of writing of the scene tree objects[p] will never be 0 here (except if p is invalid for some reason)
        objects[p]->children.insert(o);
        o->setName(name);
        o->getTransform()->setParent(objects[p]->getTransform());

        if(type == OBJECT_INSTANCE) {
            ((ktObjectInstance*)o)->setScene(retrieve<GameScene>(ref_name));
        }
        o->getTransform()->setPosition(t);
        o->getTransform()->setRotation(r);
        o->getTransform()->setScale(s);
    }

    LOG("BYTES_AVAILABLE: " << in.bytes_available());

    auto attrib_type_count = dr.read<uint32_t>();
    LOG("ATTRIB_TYPE_COUNT: " << attrib_type_count);
    for(uint32_t i = 0; i < attrib_type_count; ++i) {
        std::string type = dr.readStr();
        LOG("TYPE: " << type);
        auto attrib_count = dr.read<uint64_t>();
        LOG("ATTRIB_COUNT: " << attrib_count);
        rttr::type t = rttr::type::get_by_name(type);
        for(uint64_t j = 0; j < attrib_count; ++j) {
            uint64_t owner_id = dr.read<uint64_t>();
            uint64_t data_sz = dr.read<uint64_t>();
            assert(t.is_valid());
            auto a = objects[owner_id]->createComponent(t);
            if(!a) {
                dr.skip(data_sz);
                continue;
            }
            std::vector<char> data = dr.readArray<char>();
            dstream strm;
            strm.setBuffer(data);

            auto parent_cached = this->parent;
            this->parent = 0; // Tricking components into thinking that this is the root node,
                            // Which it is for purposes of deserialization
            a->deserialize(strm, strm.bytes_available());
            this->parent = parent_cached;
        }
    }
}

bool ktNode::write(const std::string& fname) {
    file_stream strm(fname, file_stream::F_OUT);
    if(!strm.is_open()) {
        return false;
    }

    write(strm);

    return true;
}
bool ktNode::read(const std::string& fname) {
    file_stream strm(fname, file_stream::F_IN);
    if(!strm.is_open()) {
        return false;
    }

    read(strm);

    return true;
}


void ktNode::_registerComponent(Attribute* attrib) {
    ktNode* current = parent;
    ktNode* p = 0;
    while(current) {
        p = current;
        current = current->parent;
    }
    if(p) {
        p->_registerComponent(attrib);
    }
}
void ktNode::_unregisterComponent(Attribute* attrib) {
    ktNode* current = parent;
    ktNode* p = 0;
    while(current) {
        p = current;
        current = current->parent;
    }
    if(p) {
        p->_unregisterComponent(attrib);
    }
}
void ktNode::_readdComponent(Attribute* attrib) {
    ktNode* current = parent;
    ktNode* p = 0;
    while(current) {
        p = current;
        current = current->parent;
    }
    if(p) {
        p->_readdComponent(attrib);
    }
}
