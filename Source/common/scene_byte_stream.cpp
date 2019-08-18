
#include "scene_byte_stream.hpp"

#include <stack>

#include "scene/node.hpp"
#include "scene/object_instance.hpp"
#include "scene/game_scene.hpp"

void SceneWriteCtx::write(const std::string& value) {
    strm->write<uint64_t>((uint64_t)value.size());
    strm->write(value);
}
void SceneWriteCtx::write(ktNode* node) {
    auto it = oid_map.find(node);
    if(it == oid_map.end()) {
        strm->write(std::numeric_limits<uint64_t>::max());
    } else {
        strm->write<uint64_t>(it->second);
    }
}

std::string SceneReadCtx::readStr() {
    std::string str;
    uint64_t sz = 0;
    strm->read(sz);
    strm->read(str, sz);
    return str;
}
ktNode* SceneReadCtx::readNode() {
    uint64_t node_id = strm->read<uint64_t>();
    if(node_id > objects.size()) {
        return 0;
    }
    return objects[node_id];
}
