#ifndef SCENE_SERIALIZER_HPP
#define SCENE_SERIALIZER_HPP

#include "util/data_stream.hpp"

#include "resource/resource.h"

class ktNode;
class Attribute;

class SceneWriteCtx {
public:
    SceneWriteCtx(out_stream* strm)
    : strm(strm) { }

    out_stream* strm = 0;
    std::map<ktNode*, uint64_t> oid_map;
    std::vector<ktNode*> objects;
    std::map<rttr::type, std::vector<Attribute*>> attribs;

    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const T& value);
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const std::vector<T>& values);
    void write(const std::string& value);
    template<typename T>
    void write(const std::shared_ptr<T>& resource);
    void write(ktNode* node);
};

class SceneReadCtx {
public:
    SceneReadCtx(in_stream* strm)
    : strm(strm) {}

    in_stream* strm = 0;
    std::vector<ktNode*> objects;

    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    T read();
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    std::vector<T> readArray();
    std::string readStr();
    template<typename T>
    std::shared_ptr<T> readResource();
    ktNode* readNode();
};

template<typename T, typename>
void SceneWriteCtx::write(const T& value) {
    strm->write(value);
}
template<typename T, typename>
void SceneWriteCtx::write(const std::vector<T>& values) {
    strm->write<uint64_t>((uint64_t)values.size());
    strm->write(values);
}
template<typename T>
void SceneWriteCtx::write(const std::shared_ptr<T>& resource) {
    std::string res_name;
    if(resource) {
        res_name = resource->Name();
    }
    write(res_name);
}

template<typename T, typename>
T SceneReadCtx::read() {
    T v;
    strm->read(v);
    return v;
}
template<typename T, typename>
std::vector<T> SceneReadCtx::readArray() {
    std::vector<T> vec;
    uint64_t sz = 0;
    strm->read(sz);
    strm->read(vec, sz);
    return vec;
}
template<typename T>
std::shared_ptr<T> SceneReadCtx::readResource() {
    std::string res_name = readStr();
    if(res_name.empty()) {
        return std::shared_ptr<T>();
    }
    return retrieve<T>(res_name);
}

#endif
