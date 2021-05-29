#ifndef ECS_ENTITY_STORAGE_HPP
#define ECS_ENTITY_STORAGE_HPP

#include <assert.h>
#include <stdint.h>
#include <vector>

struct EntityStorageHeader {
    uint64_t signature;
    uint64_t entity_count;
};

class ArchetypeStorage;
struct EntityChunkHeader {
    EntityChunkHeader* next_chunk;
    ArchetypeStorage* archetype;
    uint64_t chunk_id;
    uint64_t signature;
    uint64_t elem_size;
    int64_t attrib_offsets[64];

    void* getElemPtr(uint64_t idx) {
        auto base = ((uint8_t*)this) + sizeof(*this);
        return base + elem_size * idx;
    }
    void* getAttribPtr(uint64_t entity_idx, uint64_t attr_id) {
        if(attrib_offsets[attr_id] == -1) {
            return 0;
        }
        return ((uint8_t*)getElemPtr(entity_idx)) + attrib_offsets[attr_id];
    }
};
static const int ENTITY_CHUNK_HEADER_SIZE = sizeof(EntityChunkHeader);
static const int ENTITY_CHUNK_SIZE = 16 * 1024;

class EntityStorage {
    ArchetypeStorage* archetype;
    size_t elem_size;
    size_t count_per_chunk;
    std::vector<void*> chunks;
    EntityChunkHeader* last_chunk = 0;
    size_t elem_count;

    uint64_t signature;
    int64_t attrib_offsets[64];

    void* calcPtr(size_t elem_id) {
        auto chunk_id = calcChunkId(elem_id);
        auto chunk_lcl_id = calcChunkLocalId(elem_id);
        void* ptr = ((uint8_t*)chunks[chunk_id]) + (chunk_lcl_id * elem_size) + ENTITY_CHUNK_HEADER_SIZE;
        return ptr;
    }
    void allocChunk() {
        void* ptr = malloc(ENTITY_CHUNK_HEADER_SIZE + ENTITY_CHUNK_SIZE);
        chunks.push_back(ptr);

        EntityChunkHeader* header = (EntityChunkHeader*)ptr;
        header->next_chunk = last_chunk;
        header->archetype = archetype;
        header->chunk_id = chunks.size() - 1;
        header->signature = signature;
        header->elem_size = elem_size;
        memcpy(header->attrib_offsets, attrib_offsets, sizeof(attrib_offsets));
        last_chunk = header; 
    }
    void freeChunk() {
        assert(chunks.size() > 0);
        EntityChunkHeader* ptr = last_chunk;
        last_chunk = ptr->next_chunk;
        chunks.pop_back();
        free((void*)ptr);
    }
public:
    EntityStorage(ArchetypeStorage* archetype, size_t elem_size, uint64_t signature, int64_t* attrib_offsets)
    : archetype(archetype), elem_size(elem_size), count_per_chunk(ENTITY_CHUNK_SIZE / elem_size), elem_count(0) {
        assert(elem_size);
        assert(elem_size <= ENTITY_CHUNK_SIZE);

        this->signature = signature;
        memcpy(this->attrib_offsets, attrib_offsets, sizeof(this->attrib_offsets));
    }
    EntityStorage() {
        
    }
    ~EntityStorage() {
        while(chunks.size() > 0) {
            freeChunk();
        }
    }

    size_t calcChunkId(size_t elem_id) {
        return elem_id / count_per_chunk;
    }
    size_t calcChunkLocalId(size_t elem_id) {
        return elem_id % count_per_chunk;
    }
    EntityChunkHeader* getChunkHeaderPtr(size_t chunk_id) {
        return (EntityChunkHeader*)chunks[chunk_id];
    }
    uint64_t getMaxElemCountPerChunk() const {
        return count_per_chunk;
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
        memcpy(((uint8_t*)chunks[chunk_id]) + (chunk_lcl_id * elem_size) + ENTITY_CHUNK_HEADER_SIZE, data, elem_size);
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
        return ((uint8_t*)chunks[chunk_idx]) + (lcl_elem_idx * elem_size) + ENTITY_CHUNK_HEADER_SIZE;
    }
};


#endif
