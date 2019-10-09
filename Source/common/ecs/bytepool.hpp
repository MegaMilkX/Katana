#ifndef ECS_BYTEPOOL_HPP
#define ECS_BYTEPOOL_HPP

#include <vector>
#include <set>

class ktBytePool {
    size_t block_size;
    std::vector<char> data;
    std::set<size_t>  free_blocks;
public:
    ktBytePool(size_t block_size)
    : block_size(block_size) {}

    size_t acquire() {
        size_t id;
        if(!free_blocks.empty()) {
            id = *free_blocks.begin();
            free_blocks.erase(free_blocks.begin());
        } else {
            id = data.size() / block_size;
            data.resize(data.size() + block_size);
        }
        return id;
    }
    void   release(size_t id) {
        free_blocks.insert(id);
    }
    void*  deref(size_t id) {
        return data.data() + id * block_size;
    }
};

#endif
