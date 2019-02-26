#ifndef EVENT_HPP
#define EVENT_HPP

#include <queue>
#include <string>
#include <functional>
#include <map>
#include <list>

#define EVENT_MGR_MAX_QUEUE 128
#define EVENT_MAX_PAYLOAD 256

class Event {
public:
    Event(const std::string& name)
    : name(name) {

    }

    const std::string& getName() const {
        return name;
    }

    template<typename T>
    void setPayload(const T& p) {
        static_assert(sizeof(T) <= EVENT_MAX_PAYLOAD, "Can't set event payload larger than 256 bytes");
        (*(T*)payload) = p;
    }
    template<typename T>
    const T& getPayload() const {
        static_assert(sizeof(T) <= EVENT_MAX_PAYLOAD, "Event payload must be no larger than 256 bytes");
        return *(T*)payload;
    }
private:
    std::string name;
    char payload[EVENT_MAX_PAYLOAD];
};

class EventListener {
public:
    EventListener(std::function<void(const Event&)> cb)
    : cb(cb) { }
    void invoke(const Event& e) {
        cb(e);
    }
private:
    std::function<void(const Event&)> cb;    
};

class EventContext {
public:
    void post(const std::string& name) {
        if(queue.size() == EVENT_MGR_MAX_QUEUE) {
            queue.pop();
        }
        queue.push(Event(name));
    }
    template<typename T>
    void post(const std::string& name, const T& payload) {
        post(name);
        queue.back().setPayload(payload);
    }

    std::shared_ptr<EventListener> listen(const std::string& evt, std::function<void(const Event&)> cb) {
        std::shared_ptr<EventListener> ptr(new EventListener(cb));
        std::weak_ptr<EventListener> weak = ptr;
        listeners[evt].emplace_back(weak);
        return ptr;
    }

    void pollEvents() {
        size_t queue_len = queue.size();
        for(size_t i = 0; i < queue_len; ++i) {
            Event& e = queue.front();
            auto& it = listeners.find(e.getName());
            if(it != listeners.end()) {
                auto list = it->second;
                for(auto lit = list.begin(); lit != list.end(); ++lit) {
                    auto& weak = (*lit);
                    if(weak.expired()) {
                        list.erase(lit);
                        continue;
                    }
                    weak.lock()->invoke(e);
                }
            }
            queue.pop();
        }
    }
private:
    std::queue<Event> queue;
    std::map<
        std::string, 
        std::list<
            std::weak_ptr<EventListener>
        >
    > listeners;
};

class EventMgr {
public:
    EventContext& getContext(const std::string& name) {
        return contexts[name];
    }
    void pollEvents() {
        for(auto& kv : contexts) {
            kv.second.pollEvents();
        }
    }
private:
    std::map<std::string, EventContext> contexts;
};

inline EventMgr& eventMgr() {
    static EventMgr mgr;
    return mgr;
}

#endif
