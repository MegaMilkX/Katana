#ifndef KT_ARCHETYPE_HPP
#define KT_ARCHETYPE_HPP

#include <algorithm>
#include <unordered_map>
#include <vector>

#include <rttr/type>

#include "archetype_storage.hpp"

inline void ktSignatureSort(std::vector<rttr::type>& sig) {
    std::sort(sig.begin(), sig.end(), [](const rttr::type& l, const rttr::type& r)->bool{
        return l.get_id() < r.get_id();
    });
}
inline void ktSignatureSort(std::vector<std::pair<rttr::type, int>>& sig_and_offsets) {
    std::sort(sig_and_offsets.begin(), sig_and_offsets.end(), [](const std::pair<rttr::type, int>& l, const std::pair<rttr::type, int>& r)->bool{
        return l.first.get_id() < r.first.get_id();
    });
}
inline void ktSignatureRemove(std::vector<rttr::type>& sig, rttr::type type) {
    for(int i = 0; i < sig.size(); ++i) {
        if(sig[i] == type) {
            sig.erase(sig.begin() + i);
            break;
        }
    }
}
inline bool ktSignatureContainsAll(const std::vector<rttr::type>& signature, const std::vector<rttr::type>& condition_signature) {
    if(signature.size() < condition_signature.size()) {
        return false;
    }
    for(auto t : condition_signature) {
        bool found_type = false;
        for(auto t2 : signature) {
            if(t == t2) {
                found_type = true;
                break;
            }
        }
        if(!found_type) {
            return false;
        }
    }
    return true;
}

inline void ktSignatureAdd(std::vector<rttr::type>& sig, rttr::type type) {
    sig.push_back(type);
    ktSignatureSort(sig);
}

class ktArchetypeNode {
    std::unique_ptr<ktArchetypeStorage> storage;
    std::vector<int> free_slots;
public:
    std::vector<rttr::type>                 signature;
    std::vector<int>                        offsets;
    std::unordered_map<rttr::type, int>     type_to_offset;
    std::unordered_map<rttr::type, ktArchetypeNode*>    forward_nodes;
    std::unordered_map<rttr::type, ktArchetypeNode*>    backward_nodes;

    int elem_size = 0;

    ktArchetypeNode(const std::vector<rttr::type>& signature)
    : signature(signature) {
        for(auto t : signature) {
            type_to_offset[t] = offsets.size();
            offsets.push_back(elem_size);
            elem_size += t.get_sizeof();
            forward_nodes[t] = 0; // null forward nodes to check easily if we're trying to add an existing type
        }
        if(elem_size > 0) {
            storage.reset(new ktArchetypeStorage(elem_size));
        }
    }
    ktArchetypeNode(size_t elem_size) {
        this->elem_size = elem_size;
        storage.reset(new ktArchetypeStorage(elem_size));
    }
    void _lateSignatureInit(const std::vector<std::pair<rttr::type, int>>& sig) {
        assert(this->signature.empty());
        for(auto& s : sig) {
            signature.push_back(s.first);
            offsets.push_back(s.second);
            type_to_offset[s.first] = signature.size() - 1;
        }
    }

    int count() const {
        return (int)storage->count();
    }

    void* deref(int idx, rttr::type type) {
        void* base_ptr = storage->deref(idx);
        auto it = type_to_offset.find(type);
        assert(it != type_to_offset.end());
        return ((uint8_t*)base_ptr + offsets[it->second]);
    }
    void* derefBasePtr(int idx) {
        void* base_ptr = storage->deref(idx);
        return base_ptr;
    }

    bool hasType(rttr::type type) const {
        auto it = type_to_offset.find(type);
        return it != type_to_offset.end();
    }

    int allocOne() {
        if(elem_size == 0) {
            return -1;
        }
        if(!free_slots.empty()) {
            int idx = free_slots.back();
            free_slots.pop_back();
            return idx;
        } else {
            return (int)storage->alloc(); // TODO: Fix later lol
        }
    }
    void freeOne(int idx) {
        free_slots.push_back(idx);
    }
    void copyFrom(ktArchetypeNode* other, int other_idx, int tgt_idx) {
        if(other_idx < 0 || tgt_idx < 0) {
            return;
        }
        uint8_t* other_ptr = (uint8_t*)other->storage->deref(other_idx);
        uint8_t* ptr       = (uint8_t*)storage->deref(tgt_idx);
        LOG("Copying components");
        for(auto type : other->signature) {
            auto other_offset = other->offsets[other->type_to_offset.find(type)->second];
            auto it = type_to_offset.find(type);
            if(it == type_to_offset.end()) {
                LOG("(Skipped) " << type.get_name().to_string() << " " << type.get_sizeof() << " bytes");
                continue;
            }
            auto offset = offsets[it->second];
            LOG(type.get_name().to_string() << " " << type.get_sizeof() << " bytes [" << other_offset << ", " << offset << "]");
            memcpy(ptr + offset, other_ptr + other_offset, type.get_sizeof());
        }
        LOG("done.");
    }
};

class ktArchetypeGraph {
    ktArchetypeNode* null_node;
    std::vector<std::shared_ptr<ktArchetypeNode>> nodes;

    ktArchetypeNode* createArchetype(const std::vector<rttr::type>& signature) {
        nodes.push_back(std::shared_ptr<ktArchetypeNode>(new ktArchetypeNode(signature)));
        return nodes.back().get();
    }

    ktArchetypeNode* findArchetypeBrute(const std::vector<rttr::type>& signature) {
        ktArchetypeNode* node = 0;
        for(int i = 0; i < nodes.size(); ++i) {
            if(nodes[i]->signature.size() != signature.size()) {
                continue;
            }
            bool fits = true;
            for(int j = 0; j < signature.size(); ++j) {
                if(signature[j] != nodes[i]->signature[j]) {
                    fits = false;
                    break;
                }
            }
            if(fits) {
                node = nodes[i].get();
            } else {
                continue;
            }
        }
        return node;
    }

public:
    ktArchetypeGraph() {
        null_node = createArchetype(std::vector<rttr::type>());
    }

    ktArchetypeNode* getNullNode() { return null_node; }

    const decltype(ktArchetypeGraph::nodes)& getArchetypes() const {
        return nodes;
    } 

    ktArchetypeNode* getArchetype(const std::vector<rttr::type>& signature) {
        auto arch = null_node;
        for(auto t : signature) {
            arch = add(arch, t);
        }
        return arch;
    }

    ktArchetypeNode* add(ktArchetypeNode* node, rttr::type type) {
        auto it = node->forward_nodes.find(type);
        if(it != node->forward_nodes.end()) {
            return it->second; // If it->second is null - we're trying to add a type that is already a part of this archetype
        } else {
            auto& n = node->forward_nodes[type];
            auto new_sig = node->signature;
            ktSignatureAdd(new_sig, type);
            n = findArchetypeBrute(new_sig);
            if(!n) {
                n = createArchetype(new_sig);
            }
            n->backward_nodes[type] = node;
            return n;
        }
    }
    ktArchetypeNode* remove(ktArchetypeNode* node, rttr::type type) {
        auto it = node->backward_nodes.find(type);
        if(it != node->backward_nodes.end()) {
            return it->second;
        } else {
            auto& n = node->backward_nodes[type];
            auto new_sig = node->signature;
            ktSignatureRemove(new_sig, type);
            n = findArchetypeBrute(new_sig);
            if(!n) {
                n = createArchetype(new_sig);
            }
            n->forward_nodes[type] = node;
        }
    }
};


#endif
