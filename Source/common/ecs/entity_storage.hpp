#ifndef ECS_ENTITY_STORAGE_HPP
#define ECS_ENTITY_STORAGE_HPP

#include <assert.h>
#include <stdint.h>
#include <vector>

struct EntityStorageHeader {
    uint64_t signature;
    uint64_t entity_count;
};

static const int ENTITY_CHUNK_SIZE = 16 * 1024;

class EntityStorage {
    size_t elem_size;
    size_t count_per_chunk;
    std::vector<void*> chunks;
    size_t elem_count;

    size_t calcChunkId(size_t elem_id) {
        return elem_id / count_per_chunk;
    }
    size_t calcChunkLocalId(size_t elem_id) {
        return elem_id % count_per_chunk;
    }
    void* calcPtr(size_t elem_id) {
        auto chunk_id = calcChunkId(elem_id);
        auto chunk_lcl_id = calcChunkLocalId(elem_id);
        void* ptr = ((uint8_t*)chunks[chunk_id]) + (chunk_lcl_id * elem_size);
        return ptr;
    }
    void allocChunk() {
        void* ptr = malloc(ENTITY_CHUNK_SIZE);
        chunks.push_back(ptr);
    }
    void freeChunk() {
        assert(chunks.size() > 0);
        void* ptr = chunks.back();
        chunks.pop_back();
        free(ptr);
    }
public:
    EntityStorage(size_t elem_size)
    : elem_size(elem_size), count_per_chunk(ENTITY_CHUNK_SIZE / elem_size), elem_count(0) {
        assert(elem_size);
        assert(elem_size <= ENTITY_CHUNK_SIZE);
    }
    ~EntityStorage() {
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
    size_t chunkElemCount(int chunk) {
        auto a = chunk * count_per_chunk;
        return std::min(elem_count - a, count_per_chunk);
    }

    size_t insert(void* data) {
        auto chunk_id = calcChunkId(elem_count);
        if(chunks.size() < (chunk_id + 1)) {
            allocChunk();
        }
        auto chunk_lcl_id = calcChunkLocalId(elem_count);
        memcpy(((uint8_t*)chunks[chunk_id]) + (chunk_lcl_id * elem_size), data, elem_size);
        return elem_count++;
    }
    size_t alloc() {
        auto chunk_id = calcChunkId(elem_count);
        if(chunks.size() < (chunk_id + 1)) {
            allocChunk();
        }
        auto chunk_lcl_id = calcChunkLocalId(elem_count);
        return elem_count++;
    }
    void assign(size_t idx, void* data) {
        void* ptr = calcPtr(idx);
        memcpy(ptr, data, elem_size);
    }
    void erase() {
        assert(elem_count > 0);

        elem_count--;
        size_t last_index = elem_count - 1;
        if(calcChunkId(last_index) + 1 < chunks.size()) {
            freeChunk();
        }
    }
    void* deref(size_t idx) {
        return calcPtr(idx);
    }
    void* derefChunkElement(size_t chunk_idx, size_t lcl_elem_idx) {
        return ((uint8_t*)chunks[chunk_idx]) + (lcl_elem_idx * elem_size);
    }
};


#endif
