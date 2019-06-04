#ifndef FBX_READ_HPP
#define FBX_READ_HPP


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include "../../../common/util/log.hpp"
#include "../scene_node.hpp"


inline bool fbxRead(std::vector<char>& buf, SceneNode* rootNode, const std::string& fname_hint) {

    return true;
}


inline bool fbxRead(const std::string& fname, SceneNode* rootNode) {
    std::ifstream f(fname, std::ios::binary | std::ios::ate);
    if(!f.is_open()) {
        LOG_WARN("Failed to open " << fname);
        return false;
    }

    std::streamsize sz = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buf((uint64_t)sz);
    if(!f.read(buf.data(), (uint64_t)sz)) {
        f.close();
        return false;
    }

    return fbxRead(buf, rootNode, fname);
}


#endif
