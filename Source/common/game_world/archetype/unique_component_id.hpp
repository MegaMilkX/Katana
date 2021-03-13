#ifndef KT_UNIQUE_COMPONENT_ID
#define KT_UNIQUE_COMPONENT_ID


typedef int component_id;
static const component_id INVALID_COMPONENT_ID = -1;

extern component_id next_unique_component_id;
template<typename T>
struct UniqueComponentId {
    static const component_id id;
};
template<typename T>
const component_id UniqueComponentId<T>::id = next_unique_component_id++;


#endif
