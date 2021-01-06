#ifndef KT_ARCHETYPE_STORAGE_HPP
#define KT_ARCHETYPE_STORAGE_HPP

#include <assert.h>
#include <vector>

static const int ARCHETYPE_CHUNK_SIZE = 16 * 1024;

class ktArchetypeStorage {
    size_t elem_size;
    size_t count_per_chunk;
    std::vector<void*> chunks;
    size_t elem_count;

    size_t getChunkId(size_t elem_id) {
        return elem_id / count_per_chunk;
    }
    size_t getChunkLocalId(size_t elem_id) {
        return elem_id % count_per_chunk;
    }
    void* getElemPtr(size_t elem_id) {
        auto chunk_id = getChunkId(elem_id);
        auto chunk_lcl_id = getChunkLocalId(elem_id);
        void* ptr = ((uint8_t*)chunks[chunk_id]) + (chunk_lcl_id * elem_size);
        return ptr;
    }
    void allocChunk() {
        void* ptr = malloc(ARCHETYPE_CHUNK_SIZE);
        chunks.push_back(ptr);
    }
    void freeChunk() {
        assert(chunks.size() > 0);
        void* ptr = chunks.back();
        chunks.pop_back();
        free(ptr);
    }
public:
    ktArchetypeStorage(size_t elem_size)
    : elem_size(elem_size), count_per_chunk(ARCHETYPE_CHUNK_SIZE / elem_size), elem_count(0) {
        assert(elem_size);
        assert(elem_size <= ARCHETYPE_CHUNK_SIZE);
    }
    ~ktArchetypeStorage() {
        while(chunks.size() > 0) {
            freeChunk();
        }
    }

    size_t count() const {
        return elem_count;
    }
    size_t chunkCount() const {
        return chunks.size();
    }

    size_t insert(void* data) {
        auto chunk_id = getChunkId(elem_count);
        if(chunks.size() < (chunk_id + 1)) {
            allocChunk();
        }
        auto chunk_lcl_id = getChunkLocalId(elem_count);
        memcpy(((uint8_t*)chunks[chunk_id]) + (chunk_lcl_id * elem_size), data, elem_size);
        return elem_count++;
    }
    size_t alloc() {
        auto chunk_id = getChunkId(elem_count);
        if(chunks.size() < (chunk_id + 1)) {
            allocChunk();
        }
        auto chunk_lcl_id = getChunkLocalId(elem_count);
        return elem_count++;
    }
    void assign(size_t idx, void* data) {
        void* ptr = getElemPtr(idx);
        memcpy(ptr, data, elem_size);
    }
    void erase() {
        assert(elem_count > 0);

        elem_count--;
        size_t last_index = elem_count - 1;
        if(getChunkId(last_index) + 1 < chunks.size()) {
            freeChunk();
        }
    }
    void* deref(size_t idx) {
        return getElemPtr(idx);
    }
};


#endif
