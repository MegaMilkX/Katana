#include "scene.hpp"

#define MINIZ_HEADER_FILE_ONLY
#include "lib/miniz.c"

#include "util/log.hpp"
#include "util/split.hpp"
#include "util/has_suffix.hpp"

Scene* Scene::create() {
    return new Scene();
}
void Scene::destroy() {
    clear();
    delete this;
}

void Scene::clear() {
    // Root object always remains
    /*
    while(objectCount() > 1) {
        removeObject(getObject(objectCount() - 1));
    }
    getRootObject()->removeComponents();
    local_resources.clear();
    */

    for(auto& kv : components) {
        for(size_t i = 0; i < kv.second.size(); ++i) {
            triggerProbeOnRemoveRecursive(kv.first, kv.second[i].get());
        }
    }
    components.clear();
    objects.clear();

    objects.emplace_back(std::shared_ptr<SceneObject>(new SceneObject(this, 0)));
    objects.back()->setName("Root");

    scene_components.clear();
}

SceneObject* Scene::createObject(SceneObject* parent) {
    if(!parent) {
        parent = getRootObject();
    }
    objects.emplace_back(std::shared_ptr<SceneObject>(new SceneObject(this, parent)));
    if(parent) {
        parent->children.emplace_back(objects.back().get());
    }
    objects.back()->id = objects.size() - 1;
    return objects.back().get();
}

void Scene::removeObject(SceneObject* so) {
    if(!so->parent) {
        // Root object always remains
        return;
    }
    auto it = objects.end();
    for(size_t i = 0; i < objects.size(); ++i) {
        // Note the one to remove
        if(objects[i].get() == so) {
            it = objects.begin() + i;
            continue;
        }
        // If it is a parent to this object, clear the relationship
        if(objects[i]->getParent() == so) {
            objects[i]->setParent(getRootObject());
        }
    }
    if(it != objects.end()) {
        SceneObject* parent = (*it)->getParent();
        if(parent) {
            parent->removeChild((*it).get());
        }
        (*it)->removeComponents();
        objects.erase(it);
    }
}

size_t Scene::objectCount() const {
    return objects.size();
}
SceneObject* Scene::getObject(size_t index) {
    return objects[index].get();
}
SceneObject* Scene::getRootObject() {
    return objects[0].get();
}

bool zipAddFromStream(std::istream& in, mz_zip_archive& archive, const std::string& name) {
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

bool Scene::serialize(std::vector<char>& buf) {
    LOG("Serializing scene...");
    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_writer_init_heap(&archive, 0, 65537)) {
        LOG("Failed to create archive file in memory");
        return false;
    }

    // Objects
    LOG("Object count: " << objects.size());
    {
        std::stringstream ss;
        uint32_t object_count = (uint32_t)objects.size();
        ss.write((char*)&object_count, sizeof(object_count));
        for(auto so : objects) {
            so->serialize(ss);
        }

        zipAddFromStream(
            ss,
            archive,
            "objects"
        );
    }
    // Components
    {
        for(auto kv : components) {
            std::stringstream ss;
            uint32_t count = kv.second.size();
            ss.write((char*)&count, sizeof(count));

            for(auto& c : kv.second) {
                uint32_t owner_id = (uint32_t)c->scene_object->getId();
                LOG("Component owner id: " << owner_id);
                ss.write((char*)&owner_id, sizeof(owner_id));

                std::stringstream ss_c;
                c->serialize(ss_c);
                uint64_t sz = ss_c.tellp();
                ss.write((char*)&sz, sizeof(sz));
                if(sz) ss << ss_c.rdbuf();
            }

            zipAddFromStream(
                ss, 
                archive,
                MKSTR(kv.first.get_name().to_string() << ".comp_array")
            );
        }
    }
    // Local resources
    {
        for(auto kv : this->local_resources) {
            std::stringstream ss;
            uint32_t count = kv.second.size();
            ss.write((char*)&count, sizeof(count));

            for(auto& r : kv.second) {
                std::stringstream ss_r;
                r->serialize(ss_r);
                uint64_t sz = ss_r.tellp();
                ss.write((char*)&sz, sizeof(sz));
                if(sz) ss << ss_r.rdbuf();
            }

            zipAddFromStream(
                ss, 
                archive, 
                MKSTR(kv.first.get_name().to_string() << ".res_array")
            );
        }
    }

    void* archbuf = 0;
    size_t sz = 0;
    mz_zip_writer_finalize_heap_archive(&archive, &archbuf, &sz);
    buf = std::vector<char>((char*)archbuf, (char*)archbuf + sz);
    mz_zip_writer_end(&archive);

    LOG("Done");
    return true;
}

bool Scene::serialize(const std::string& filename) {
    std::vector<char> buf;
    if(!serialize(buf)) {
        return false;
    }

    std::ofstream f(filename, std::ios::binary);
    if(!f.is_open()) {
        return false;
    }
    f.write(buf.data(), buf.size());
    f.close();

    return true;
}

bool zipExtract(mz_zip_archive& arch, std::vector<char>& data, const std::string& filename) {
    int file_id = mz_zip_reader_locate_file(&arch, "objects", 0, 0);
    if(file_id < 0) {
        LOG_ERR("Failed to locate 'objects' file");
        return false;
    }
    mz_zip_archive_file_stat file_stat;
    if(!mz_zip_reader_file_stat(&arch, file_id, &file_stat)) {
        LOG_ERR("Failed to get file_stat for 'objects'");
        return false;
    }

    data.resize(file_stat.m_uncomp_size);

    if(!mz_zip_reader_extract_to_mem(&arch, file_id, (void*)data.data(), data.size(), 0)) {
        LOG_ERR("Failed to extract 'objects'");
        return false;
    }
    return true;
}

bool Scene::deserialize(std::vector<char>& data) {
    LOG("Deserializing scene...")
    clear();

    mz_zip_archive archive;
    memset(&archive, 0, sizeof(archive));
    if(!mz_zip_reader_init_mem(&archive, data.data(), data.size(), 0)) {
        LOG_ERR("mz_zip_reader_init_mem failed");
        return false;
    }

    //
    {
        std::vector<char> buf_objects;
        zipExtract(archive, buf_objects, "objects");
        std::stringstream ss;
        ss.write(buf_objects.data(), buf_objects.size());
        uint32_t object_count = 0;
        ss.read((char*)&object_count, sizeof(object_count));
        LOG("Object count to read: " << object_count);

        for(uint32_t i = 1; i < object_count; ++i) {
            SceneObject* so = createObject();
        }
        for(uint32_t i = 0; i < object_count; ++i) {
            objects[i]->deserialize(ss);
        }
        objects[0]->parent = 0;
    }

    std::vector<mz_uint> resource_array_file_indices;
    std::vector<mz_uint> component_array_file_indices;

    mz_uint num_files = mz_zip_reader_get_num_files(&archive);
    for(mz_uint i = 0; i < num_files; ++i) {
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            LOG_ERR("Failed to get file stat for file at index " << i);
            continue;
        }
        std::string z_filename = file_stat.m_filename;
        
        if(has_suffix(z_filename, ".comp_array")) {
            component_array_file_indices.emplace_back(i);            
            continue;
        } else if(has_suffix(z_filename, ".res_array")) {
            resource_array_file_indices.emplace_back(i);
            continue;
        }
    }

    for(auto i : resource_array_file_indices) {
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            LOG_ERR("Failed to get file stat for file at index " << i);
            continue;
        }
        std::string z_filename = file_stat.m_filename;

        // Reading resource array
        std::vector<char> buf(file_stat.m_uncomp_size);
        if(!mz_zip_reader_extract_to_mem(&archive, i, (void*)buf.data(), file_stat.m_uncomp_size, 0)) {
            LOG_WARN("Failed to extract file '" << z_filename << "'");
            continue;
        }
        std::vector<std::string> tokens = split(z_filename, '.');
        if(tokens.size() != 2) {
            LOG_WARN("Invalid component file name: " << z_filename);
            continue;
        }
        std::string res_name = tokens[0];

        std::stringstream ss;
        ss.write(buf.data(), buf.size());
        uint32_t res_count = 0;
        ss.read((char*)&res_count, sizeof(res_count));
        LOG("Loading " << res_count << " " << res_name << " local resources");
        rttr::type type = rttr::type::get_by_name(res_name);
        if(!type.is_valid()) {
            LOG_WARN(res_name << " is not a valid type");
            continue;
        }
        for(uint32_t i = 0; i < res_count; ++i) {
            uint64_t sz = 0;
            ss.read((char*)&sz, sizeof(sz));
            
            // TODO:
            rttr::variant v = type.create();
            if(!v.get_type().is_pointer()) {
                LOG_WARN(res_name << " - rttr constructor policy must be pointer (" << v.get_type().get_name().to_string() << ")");
                break;
            }
            Resource* res = v.get_value<Resource*>();
            std::shared_ptr<Resource> res_ptr(res);
            res_ptr->Storage(Resource::LOCAL);
            res_ptr->Name(MKSTR(res_name << "#" << i));
            res_ptr->deserialize(ss, (size_t)sz);
            addLocalResource(type, res_ptr);
        }
    }

    for(auto i : component_array_file_indices) {
        mz_zip_archive_file_stat file_stat;
        if(!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            LOG_ERR("Failed to get file stat for file at index " << i);
            continue;
        }
        std::string z_filename = file_stat.m_filename;

        // Reading a component array
        std::vector<char> buf(file_stat.m_uncomp_size);
        if(!mz_zip_reader_extract_to_mem(&archive, i, (void*)buf.data(), file_stat.m_uncomp_size, 0)) {
            LOG_WARN("Failed to extract file '" << z_filename << "'");
            continue;
        }
        std::vector<std::string> tokens = split(z_filename, '.');
        if(tokens.size() != 2) {
            LOG_WARN("Invalid component file name: " << z_filename);
            continue;
        }
        std::string component_name = tokens[0];
        
        std::stringstream ss;
        ss.write(buf.data(), buf.size());
        uint32_t component_count = 0;
        ss.read((char*)&component_count, sizeof(component_count));
        LOG("Loading " << component_count << " " << component_name);
        for(uint32_t i = 0; i < component_count; ++i) {
            uint32_t owner_id = 0;
            ss.read((char*)&owner_id, sizeof(owner_id));
            uint64_t sz = 0;
            ss.read((char*)&sz, sizeof(sz));

            LOG("Component owner_id: " << owner_id);
            LOG("Component owner ptr: " << getObject(owner_id));
            if(getObject(owner_id)) {
                Component* c = getObject(owner_id)->get(component_name);
                if(!c) {
                    LOG_WARN("Failed to create component " << component_name);
                    break;
                }
                c->deserialize(ss, (size_t)sz);
            } else {
                LOG_ERR("No owner found for id " << owner_id);
                break;
            }
        }
    }

    mz_zip_reader_end(&archive);
    return true;
}
bool Scene::deserialize(const std::string& filename) {
    std::ifstream f(filename, std::ios::binary | std::ios::ate);
    if(!f.is_open()) {
        std::cout << "Failed to open " << filename << std::endl;
        return false;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buffer((unsigned int)size);
    if(!f.read(buffer.data(), (unsigned int)size)) {
        f.close();
        return false;
    }

    deserialize(buffer);
    
    f.close();
    return true;
}