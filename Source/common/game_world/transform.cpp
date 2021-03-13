#include "transform.hpp"

#include <vector>

static uint32_t next_version = 0;
static std::vector<TransformData> data_array;
static std::vector<uint32_t> version_array;
static std::vector<uint32_t> free_slots;

hTransform createTransformNode() {
    if(!free_slots.empty()) {
        auto free_slot = free_slots.back();
        free_slots.resize(free_slots.size() - 1);
        return hTransform(free_slot, version_array[free_slot]);
    } else {
        uint32_t index = data_array.size();
        uint32_t version = next_version;
        data_array.push_back(TransformData());
        version_array.push_back(next_version++);
        return hTransform(index, version);
    }
}

void destroyTransformNode(hTransform hdl) {
    version_array[hdl.getIndex()] = next_version++;
    free_slots.push_back(hdl.getIndex());
}


TransformData* hTransform::operator->() {
    if(version_array[index] != version || index == INVALID_TRANSFORM_INDEX) {
        return 0;
    }
    return &data_array[index];
}
const TransformData* hTransform::operator->() const {
    if(version_array[index] != version || index == INVALID_TRANSFORM_INDEX) {
        return 0;
    }
    return &data_array[index];
}

hTransform::operator bool () const {
    return index != INVALID_TRANSFORM_INDEX && version_array[index] == version;
}