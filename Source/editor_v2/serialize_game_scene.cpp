#include "serialize_game_scene.hpp"

#include "../common/util/zip_writer.hpp"
#include "../common/util/zip_reader.hpp"

static void serializeObject(std::ostream& out, GameObject* o) {
    ZipWriter zipw;
    if(!zipw.isValid()) {
        LOG_WARN("ZipWriter is invalid");
        return;
    }

    auto name = o->getName();
    uint64_t uid = (uint64_t)o;
    gfxm::mat4 t = o->getTransform()->getLocalTransform();
    gfxm::aabb box = o->getAabb();

    zipw.add("name", name);
    zipw.add("uid", uid);
    zipw.add("transform", t);
    zipw.add("aabb", box);

    std::stringstream component_stream;
    o->serializeComponents(component_stream);
    zipw.add("components", component_stream);

    if(o->getParent()) {
        uint64_t puid = (uint64_t)o->getParent();
        zipw.add("parent", puid);
    }
    std::vector<uint64_t> child_uids;
    child_uids.emplace_back(o->childCount());
    for(size_t i = 0; i < o->childCount(); ++i) {
        child_uids.emplace_back((uint64_t)o->getChild(i).get());
    }
    zipw.add("children", child_uids);
    zipw.add("type", o->get_type().get_name().to_string());
    
    std::stringstream ss;
    o->serialize(ss);
    zipw.add("object_data", ss);

    std::vector<char> buf = zipw.finalize();
    out.write(buf.data(), buf.size());
}

static void serializeObjectRecursive(ZipWriter& zipw, GameObject* o) {
    std::stringstream ss;
    serializeObject(ss, o);

    zipw.add(MKSTR(o), ss);

    for(size_t i = 0; i < o->childCount(); ++i) {
        auto& c = o->getChild(i);
        serializeObjectRecursive(zipw, c.get());
    }
}

static bool serializeObjects(std::vector<char>& buf, GameScene* scn) {
    LOG("Serializing objects...");
    ZipWriter zip_writer;

    for(size_t i = 0; i < scn->objectCount(); ++i) {
        auto o = scn->getObject(i);
        serializeObjectRecursive(zip_writer, o);
    }

    buf = zip_writer.finalize();
    LOG("Done");
    return true;
}

static bool serializeControllers(out_stream& out, GameScene* scn) {
    ZipWriter zw;
    for(size_t i = 0; i < scn->controllerCount(); ++i) {
        SceneController* con = scn->getController(i);
        dstream cstrm;
        con->serialize(cstrm);
        zw.add(con->get_type().get_name().to_string(), cstrm.getBuffer());
    }

    out.write(zw.finalize());
    return true;
}

static bool serializeScene(std::vector<char>& buf, GameScene* scn) {
    LOG("Serializing scene...");

    ZipWriter zip_writer;

    std::vector<char> objects_buf;
    serializeObjects(objects_buf, scn);
    zip_writer.add("objects", objects_buf);

    dstream ctrl_stream;
    serializeControllers(ctrl_stream, scn);
    zip_writer.add("controllers", ctrl_stream.getBuffer());

    buf = zip_writer.finalize();
    LOG("Done");
    return true;
}

bool serializeScene(const std::string& fname, GameScene* scn) {
    std::vector<char> buf;
    if(!serializeScene(buf, scn)) {
        return false;
    }
    std::ofstream f(fname, std::ios::binary);
    if(!f.is_open()) {
        return false;
    }
    f.write(buf.data(), buf.size());
    f.close();
    return true;
}

struct ObjectTemp {
    std::string name;
    std::string type;
    uint64_t uid;
    uint64_t parent;
    std::vector<uint64_t> children;
    gfxm::mat4 transform;
    gfxm::aabb aabb;
    std::vector<char> object_data;
    std::vector<char> component_data;
    GameObject* real_object;
};

static bool deserializeObject(std::map<uint64_t, ObjectTemp>& objects, const std::vector<char>& buf, GameScene* scn) {
    ZipReader zipr(buf);

    std::vector<char> buf_components = zipr.extractFile("components");
    std::vector<char> buf_object_data = zipr.extractFile("object_data");
    
    std::string name = zipr.extractString("name");
    uint64_t uid = zipr.extract<uint64_t>("uid");
    gfxm::mat4 t = zipr.extract<gfxm::mat4>("transform");
    gfxm::aabb box = zipr.extract<gfxm::aabb>("aabb");
    uint64_t parent = zipr.extract<uint64_t>("parent");

    std::vector<char> buf_children = zipr.extractFile("children");
    uint64_t child_count = *(uint64_t*)buf_children.data();
    std::vector<uint64_t> children((uint64_t*)buf_children.data() + 1, (uint64_t*)buf_children.data() + child_count + 1);
    std::string type = zipr.extractString("type");
    
    objects[uid] = ObjectTemp {
        name, type, uid, parent, children, t, box, buf_object_data, buf_components
    };
    return true;
}

static void deserializeComponents(GameObject* o, std::vector<char>& data) {
    ZipReader zipr(data);

    size_t num_files = zipr.fileCount();
    for(size_t i = 0; i < num_files; ++i) {
        std::string z_filename = zipr.getFileName(i);
        rttr::type t = rttr::type::get_by_name(z_filename);
        if(!t.is_valid()) {
            LOG("Failed to load component of type '" << z_filename << "' (no such type)");
            continue;
        }
        auto comp = o->get(t);
        
        std::vector<char> file_data = zipr.extractFile(i);
        
        dstream ds;
        ds.setBuffer(file_data);
        comp->deserialize(ds, ds.bytes_available());
    }
}

static void createObjectTree(std::map<uint64_t, ObjectTemp>& objects, uint64_t child_uid, GameObject* parent) {
    ObjectTemp& child_data = objects[child_uid];

    rttr::type t = rttr::type::get_by_name(child_data.type);
    if(!t.is_valid()) t = rttr::type::get<GameObject>();
    auto new_o = parent->createChild(t);
    child_data.real_object = new_o.get();
    new_o->setName(child_data.name);
    new_o->setAabb(child_data.aabb);
    new_o->getTransform()->setTransform(child_data.transform);
    std::stringstream ss;
    ss.write(child_data.object_data.data(), child_data.object_data.size());

    for(size_t i = 0; i < child_data.children.size(); ++i) {
        createObjectTree(objects, child_data.children[i], new_o.get());
    }

    new_o->deserialize(ss, child_data.object_data.size());
}

static void createObjectTree(std::map<uint64_t, ObjectTemp>& objects, const std::vector<uint64_t>& roots, GameScene* scn) {
    for(auto r : roots) {
        rttr::type t = rttr::type::get_by_name(objects[r].type);
        if(!t.is_valid()) t = rttr::type::get<GameObject>();
        auto o = scn->create(t);
        objects[r].real_object = o;
        o->setName(objects[r].name);
        o->setAabb(objects[r].aabb);
        o->getTransform()->setTransform(objects[r].transform);

        for(size_t i = 0; i < objects[r].children.size(); ++i) {
            createObjectTree(objects, objects[r].children[i], o);
        }
    }
}

static void createComponents(std::map<uint64_t, ObjectTemp>& objects) {
    for(auto& kv : objects) {
        deserializeComponents(kv.second.real_object, kv.second.component_data);
    }
}

static void fillObjects(std::map<uint64_t, ObjectTemp>& objects) {
    for(auto& kv : objects) {
        std::stringstream ss;
        ss.write(kv.second.object_data.data(), kv.second.object_data.size());
        kv.second.real_object->deserialize(ss, kv.second.object_data.size());
    }
}

static bool deserializeObjects(const std::vector<char>& buf, GameScene* scn) {
    ZipReader zipr(buf);

    std::map<uint64_t, ObjectTemp> objects;

    size_t num_files = zipr.fileCount();
    for(size_t i = 0; i < num_files; ++i) {
        std::string z_filename = zipr.getFileName(i);
        std::vector<char> file_data = zipr.extractFile(i);
        deserializeObject(objects, file_data, scn);
    }

    std::vector<uint64_t> roots;
    for(auto& kv : objects) {
        if(objects.count(kv.second.parent) == 0) {
            roots.emplace_back(kv.second.uid);
        }
    }
    createObjectTree(objects, roots, scn);
    createComponents(objects);
    fillObjects(objects);
}

static bool deserializeControllers(const std::vector<char>& buf, GameScene* scn) {
    ZipReader zr(buf);
    for(size_t i = 0; i < zr.fileCount(); ++i) {
        std::string tname = zr.getFileName(i);
        rttr::type t = rttr::type::get_by_name(tname);
        if(!t.is_valid()) {
            LOG_WARN("Invalid controller type: " << tname);
            continue;
        }
        SceneController* ctrl = scn->getController(t);
        if(!ctrl) {
            LOG_WARN("Failed to get controller " << tname);
            continue;
        }
        dstream cstrm;
        cstrm.setBuffer(zr.extractFile(tname));
        ctrl->deserialize(cstrm);
    }

    return true;
}

bool deserializeScene(const std::vector<char>& buf, GameScene* scn) {
    ZipReader zipr(buf);

    std::vector<char> objects_buf = zipr.extractFile("objects");
    deserializeObjects(objects_buf, scn);

    deserializeControllers(zipr.extractFile("controllers"), scn);
    
    return true;
}

bool deserializeScene(const std::string& fname, GameScene* scn) {
    std::ifstream f(fname, std::ios::binary | std::ios::ate);
    if(!f.is_open()) {
        LOG_WARN("Failed to open " << fname);
        return false;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer((unsigned int)size);
    if(!f.read(buffer.data(), (unsigned int)size)) {
        f.close();
        return false;
    }

    deserializeScene(buffer, scn);
        
    f.close();
    return true;
}