
#include "scene_byte_stream.hpp"



void SceneByteStream::write(const std::string& value) {
    out_stream& o = strm;
    o.write<uint64_t>((uint64_t)value.size());
    o.write(value);
}
void SceneByteStream::write(ktNode* node) {
    out_stream& o = strm;
    auto it = oid_map.find(node);
    if(it == oid_map.end()) {
        o.write(std::numeric_limits<uint64_t>::max());
    } else {
        o.write<uint64_t>(it->second);
    }
}

ktNode* SceneByteStream::readNode() {
    in_stream& in = strm;
    uint64_t node_id = in.read<uint64_t>();
    if(node_id > objects.size()) {
        return 0;
    }
    return objects[node_id];
}