#ifndef ECS_ACTOR_HPP
#define ECS_ACTOR_HPP

#include <map>
#include <memory>
#include <stdint.h>
#include <rttr/type>

#include "../attribute.hpp"


class ecsActor {
public:
    virtual ~ecsActor() {}
};


struct ActorDesc {
    uint64_t signature;
    int64_t offset_table[64];
};

typedef std::map<rttr::type, std::unique_ptr<ActorDesc>> actor_desc_map_t;

actor_desc_map_t& getActorDescMap();

template<typename ACTOR_T>
ActorDesc* actorGetDesc() {
    auto& map = getActorDescMap();
    auto it = map.find(rttr::type::get<ACTOR_T>());
    if(it == map.end()) {
        return 0;
    }
    return it->second.get();
}

template<typename ACTOR_T>
void actorSetDesc(const ActorDesc& desc) {
    auto& map = getActorDescMap();
    map.insert(std::make_pair(rttr::type::get<ACTOR_T>(), std::unique_ptr<ActorDesc>(new ActorDesc(desc))));
} 

template<typename ACTOR_T>
class ActorDescBuilder {
    uint64_t signature = 0;
    int64_t offset_table[64];
    ACTOR_T tmp_actor_instance;
public:
    ActorDescBuilder(const char* name) {
        std::fill(offset_table, offset_table + sizeof(offset_table) / sizeof(offset_table[0]), -1);
    }
    ~ActorDescBuilder() {
        ActorDesc desc;
        desc.signature = signature;
        memcpy(desc.offset_table, offset_table, sizeof(offset_table));
        getActorDescMap().insert(std::make_pair(rttr::type::get<ACTOR_T>(), std::unique_ptr<ActorDesc>(new ActorDesc(desc)) ));
    }
    template<typename ATTRIB_T>
    ActorDescBuilder& attrib(ATTRIB_T ACTOR_T::*ptr) {
        auto attrib_id = ATTRIB_T::get_id_static();
        uint64_t sig = 1ULL << attrib_id;
        signature |= sig;

        offset_table[attrib_id] = int(size_t(&(tmp_actor_instance.*ptr)) - size_t(&tmp_actor_instance));
        return *this;
    }
};


#endif
