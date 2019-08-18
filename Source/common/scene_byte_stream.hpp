#ifndef SCENE_SERIALIZER_HPP
#define SCENE_SERIALIZER_HPP

#include "data_stream.hpp"

#include "resource/resource.h"
#include "scene/node.hpp"

class SceneByteStream {
public:
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const T& value);
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const std::vector<T>& values);
    void write(const std::string& value);
    template<typename T>
    void write(const std::shared_ptr<T>& resource);
    void write(ktNode* node);

    template<typename T>
    T read();
    template<typename T>
    std::vector<T> readArray();
    template<typename T>
    std::string readStr();
    template<typename T>
    std::shared_ptr<T> readResource();
    ktNode* readNode();
private:
    dstream strm;

    std::map<ktNode*, uint64_t> oid_map;
    std::vector<ktNode*> objects;
    std::map<rttr::type, std::vector<Attribute*>> attribs;
};

template<typename T, typename>
void SceneByteStream::write(const T& value) {
    out_stream& o = strm;
    o.write(value);
}
template<typename T, typename>
void SceneByteStream::write(const std::vector<T>& values) {
    out_stream& o = strm;
    o.write<uint64_t>((uint64_t)values.size());
    o.write(values);
}
template<typename T>
void SceneByteStream::write(const std::shared_ptr<T>& resource) {
    std::string res_name;
    if(resource) {
        res_name = resource->getName();
    }
    write(res_name);
}

template<typename T>
T SceneByteStream::read() {
    in_stream& in = strm;
    T v;
    in.read(v);
    return v;
}
template<typename T>
std::vector<T> SceneByteStream::readArray() {
    in_stream& in = strm;
    std::vector<T> vec;
    uint64_t sz = 0;
    in.read(sz);
    in.read(vec, sz);
    return vec;
}
template<typename T>
std::string SceneByteStream::readStr() {
    in_stream& in = strm;
    std::string str;
    uint64_t sz = 0;
    in.read(sz);
    in.read(str, sz);
    return str;
}
template<typename T>
std::shared_ptr<T> SceneByteStream::readResource() {
    std::string res_name = readStr();
    return retrieve<T>(res_name);
}

#endif
