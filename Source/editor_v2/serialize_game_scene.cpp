#include "serialize_game_scene.hpp"

#define MINIZ_HEADER_FILE_ONLY
#include "../common/lib/miniz.c"

static bool zipAddFromBuf(std::vector<char>& buf, mz_zip_archive& archive, const std::string& name) {    
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

static bool zipAddFromStream(std::istream& in, mz_zip_archive& archive, const std::string& name) {
    in.seekg(0, std::ios::end);
    size_t sz = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<char> buf;
    buf.resize(sz);
    in.read((char*)buf.data(), sz);
    
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

static void serializeObject(std::ostream& out, GameObject* o) {
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_writer_init_heap(&archive, 0, 65537)) {
        LOG("Failed to create archive file in memory");
        return;
    }

    auto name = o->getName();
    uint64_t uid = o->getUid();
    gfxm::mat4 t = o->getTransform()->getLocalTransform();
    gfxm::aabb box = o->getAabb();

    zipAdd((void*)name.data(), name.size(), archive, "name");
    zipAdd((void*)&uid, sizeof(uid), archive, "uid");
    zipAdd((void*)&t, sizeof(t), archive, "transform");
    zipAdd((void*)&box, sizeof(box), archive, "aabb");
    std::stringstream component_stream;
    o->serializeComponents(component_stream);
    zipAddFromStream(component_stream, archive, "components");
    if(o->getParent()) {
        uint64_t puid = o->getParent()->getUid();
        zipAdd((void*)&puid, sizeof(puid), archive, "parent");
    }
    std::vector<uint64_t> child_uids;
    child_uids.emplace_back(o->childCount());
    for(size_t i = 0; i < o->childCount(); ++i) {
        child_uids.emplace_back(o->getChild(i)->getUid());
    }
    zipAdd((void*)child_uids.data(), child_uids.size() * sizeof(uint64_t), archive, "children");
    zipAdd((void*)o->get_type().get_name().to_string().data(), o->get_type().get_name().to_string().size(), archive, "type");
    
    std::stringstream ss;
    o->serialize(ss);
    zipAddFromStream(ss, archive, "object_data");

    void* archbuf = 0;
    size_t sz = 0;
    mz_zip_writer_finalize_heap_archive(&archive, &archbuf, &sz);
    std::vector<char> buf = std::vector<char>((char*)archbuf, (char*)archbuf + sz);
    mz_zip_writer_end(&archive);

    out.write(buf.data(), buf.size());
}

static void serializeObjectRecursive(mz_zip_archive& archive, GameObject* o) {
    std::stringstream ss;
    serializeObject(ss, o);
    zipAddFromStream(ss, archive, MKSTR(o->getUid()));

    for(size_t i = 0; i < o->childCount(); ++i) {
        auto& c = o->getChild(i);
        serializeObjectRecursive(archive, c.get());
    }
}

static bool serializeObjects(std::vector<char>& buf, GameScene* scn) {
    LOG("Serializing objects...");
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_writer_init_heap(&archive, 0, 65537)) {
        LOG("Failed to create archive file in memory");
        return false;
    }

    for(size_t i = 0; i < scn->objectCount(); ++i) {
        auto o = scn->getObject(i);
        serializeObjectRecursive(archive, o);
    }

    void* archbuf = 0;
    size_t sz = 0;
    mz_zip_writer_finalize_heap_archive(&archive, &archbuf, &sz);
    buf = std::vector<char>((char*)archbuf, (char*)archbuf + sz);
    mz_zip_writer_end(&archive);
    LOG("Done");
    return true;
}

static bool serializeScene(std::vector<char>& buf, GameScene* scn) {
    LOG("Serializing scene...");
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_writer_init_heap(&archive, 0, 65537)) {
        LOG("Failed to create archive file in memory");
        return false;
    }

    std::vector<char> objects_buf;
    serializeObjects(objects_buf, scn);
    zipAddFromBuf(objects_buf, archive, "objects");

    // TODO

    void* archbuf = 0;
    size_t sz = 0;
    mz_zip_writer_finalize_heap_archive(&archive, &archbuf, &sz);
    buf = std::vector<char>((char*)archbuf, (char*)archbuf + sz);
    mz_zip_writer_end(&archive);
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

static bool zipExtract(mz_zip_archive& arch, std::vector<char>& data, const std::string& filename) {
    int file_id = mz_zip_reader_locate_file(&arch, filename.c_str(), 0, 0);
    if(file_id < 0) {
        LOG_ERR("Failed to locate '" << filename << "' file");
        return false;
    }
    mz_zip_archive_file_stat file_stat;
    if(!mz_zip_reader_file_stat(&arch, file_id, &file_stat)) {
        LOG_ERR("Failed to get file_stat for '" << filename << "'");
        return false;
    }

    data.resize(file_stat.m_uncomp_size);

    if(!mz_zip_reader_extract_to_mem(&arch, file_id, (void*)data.data(), data.size(), 0)) {
        LOG_ERR("Failed to extract '" << filename << "'");
        return false;
    }
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
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_reader_init_mem(&archive, buf.data(), buf.size(), 0)) {
        LOG_ERR("mz_zip_reader_init_mem failed");
        return false;
    }

    std::vector<char> buf_name;
    zipExtract(archive, buf_name, "name");
    std::vector<char> buf_uid;
    zipExtract(archive, buf_uid, "uid");
    std::vector<char> buf_transform;
    zipExtract(archive, buf_transform, "transform");
    std::vector<char> buf_aabb;
    zipExtract(archive, buf_aabb, "aabb");
    std::vector<char> buf_parent;
    zipExtract(archive, buf_parent, "parent");
    std::vector<char> buf_children;
    zipExtract(archive, buf_children, "children");
    std::vector<char> buf_type;
    zipExtract(archive, buf_type, "type");
    std::vector<char> buf_components;
    zipExtract(archive, buf_components, "components");
    std::vector<char> buf_object_data;
    zipExtract(archive, buf_object_data, "object_data");
    
    std::string name(buf_name.data(), buf_name.data() + buf_name.size());
    uint64_t uid = *(uint64_t*)buf_uid.data();
    gfxm::mat4 t = *(gfxm::mat4*)buf_transform.data();
    gfxm::aabb box = *(gfxm::aabb*)buf_aabb.data();
    uint64_t parent = 0;
    if(!buf_parent.empty()) {
        parent = *(uint64_t*)buf_parent.data();
    }
    uint64_t child_count = *(uint64_t*)buf_children.data();
    std::vector<uint64_t> children((uint64_t*)buf_children.data() + 1, (uint64_t*)buf_children.data() + child_count + 1);
    std::string type(buf_type.data(), buf_type.data() + buf_type.size());

    objects[uid] = ObjectTemp {
        name, type, uid, parent, children, t, box, buf_object_data, buf_components
    };

    mz_zip_reader_end(&archive);
    return true;
}

static void deserializeComponents(GameObject* o, std::vector<char>& data) {
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_reader_init_mem(&archive, data.data(), data.size(), 0)) {
        LOG_ERR("mz_zip_reader_init_mem failed");
        return;
    }
    mz_uint num_files = mz_zip_reader_get_num_files(&archive);
    for(mz_uint i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            LOG_ERR("Failed to get file stat for file at index " << i);
            continue;
        }
        std::string z_filename = file_stat.m_filename;
        rttr::type t = rttr::type::get_by_name(z_filename);
        if(!t.is_valid()) {
            LOG("Failed to load component of type '" << z_filename << "' (no such type)");
            continue;
        }
        auto comp = o->get(t);
        
        std::vector<char> file_data;
        file_data.resize(file_stat.m_uncomp_size);
        if(!mz_zip_reader_extract_to_mem(&archive, i, (void*)file_data.data(), file_stat.m_uncomp_size, 0)) {
            LOG_WARN("Failed to extract file '" << z_filename << "'");
            continue;
        }
        
        std::stringstream ss;
        ss.write(file_data.data(), file_data.size());
        comp->deserialize(ss, file_data.size());
    }
    mz_zip_reader_end(&archive);
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
        std::stringstream ss;
        ss.write(objects[r].object_data.data(), objects[r].object_data.size());

        for(size_t i = 0; i < objects[r].children.size(); ++i) {
            createObjectTree(objects, objects[r].children[i], o);
        }
        
        o->deserialize(ss, objects[r].object_data.size());
    }
}

static void createComponents(std::map<uint64_t, ObjectTemp>& objects) {
    for(auto& kv : objects) {
        deserializeComponents(kv.second.real_object, kv.second.component_data);
    }
}

static bool deserializeObjects(const std::vector<char>& buf, GameScene* scn) {
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_reader_init_mem(&archive, buf.data(), buf.size(), 0)) {
        LOG_ERR("mz_zip_reader_init_mem failed");
        return false;
    }

    std::map<uint64_t, ObjectTemp> objects;

    mz_uint num_files = mz_zip_reader_get_num_files(&archive);
    for(mz_uint i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            LOG_ERR("Failed to get file stat for file at index " << i);
            continue;
        }
        std::string z_filename = file_stat.m_filename;
        std::vector<char> file_data;
        file_data.resize(file_stat.m_uncomp_size);
        if(!mz_zip_reader_extract_to_mem(&archive, i, (void*)file_data.data(), file_stat.m_uncomp_size, 0)) {
            LOG_WARN("Failed to extract file '" << z_filename << "'");
            continue;
        }
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

    mz_zip_reader_end(&archive);
}

bool deserializeScene(const std::vector<char>& buf, GameScene* scn) {
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_reader_init_mem(&archive, buf.data(), buf.size(), 0)) {
        LOG_ERR("mz_zip_reader_init_mem failed");
        return false;
    }

    std::vector<char> objects_buf;
    zipExtract(archive, objects_buf, "objects");
    deserializeObjects(objects_buf, scn);

    mz_zip_reader_end(&archive);
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