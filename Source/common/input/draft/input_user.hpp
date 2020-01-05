#ifndef INPUT_USER_HPP
#define INPUT_USER_HPP


#include <unordered_map>
#include <memory>

#include <rttr/type>

#include "input_types.hpp"
#include "adapters/input_adapter.hpp"


struct InputUserLocalActionState {
    bool is_pressed = false;
    float press_time = .0f;
};

struct InputUserLocalAxisState {
    float value = .0f;
};

class InputUser {
    std::unordered_map<rttr::type, std::shared_ptr<InputAdapter>> adapters; 
    std::vector<InputUserLocalActionState> action_state_buffer;
    std::vector<InputUserLocalAxisState> axis_state_buffer;

public:
    void resizeStateBuffers(size_t action_count, size_t axis_count) {
        action_state_buffer.resize(action_count);
        axis_state_buffer.resize(axis_count);
    }
    void clearAdapterKeyTables() {
        for(auto& kv : adapters) {
            kv.second->clear();
        }
    }

    InputUserLocalActionState& getLocalActionState(input_action_uid_t uid) {
        return action_state_buffer[uid - 1];
    }
    InputUserLocalAxisState& getLocalAxisState(input_axis_uid_t uid) {
        return axis_state_buffer[uid - 1];
    }

    template<typename ADAPTER_T>
    ADAPTER_T* getAdapter() {
        rttr::type t = rttr::type::get<ADAPTER_T>();
        auto it = adapters.find(t);
        if(it == adapters.end()) {
            it = adapters.insert(adapters.begin(), std::make_pair(t, std::shared_ptr<InputAdapter>(new ADAPTER_T)));
        }
        return (ADAPTER_T*)it->second.get();
    }
    InputAdapter* getAdapter(rttr::type type) {
        auto it = adapters.find(type);
        if(it == adapters.end()) {
            return 0;
        }
        return it->second.get();
    }
};


#endif
