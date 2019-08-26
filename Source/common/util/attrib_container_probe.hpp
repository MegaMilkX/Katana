#ifndef ATTRIB_CONTAINER_PROBE
#define ATTRIB_CONTAINER_PROBE

#include "../scene/scene_controller.hpp"

template<typename T>
class ktAttribSetProbe : public SceneEventFilter<T> {
    std::set<T*> attribs;

    void onAttribCreated(T* a) { attribs.insert(a); }
    void onAttribRemoved(T* a) { attribs.erase(a); }

public:
    std::set<T*>& getContainer() { return attribs; }
};

template<typename T>
class ktAttribVectorProbe : public SceneEventFilter<T> {
    std::vector<T*> attribs;

    void onAttribCreated(T* a) { 
        for(size_t i = 0; i < attribs.size(); ++i) {
            if(attribs[i] == a) {
                return;
            }
        }
        attribs.emplace_back(a); 
    }
    void onAttribRemoved(T* a) { 
        for(size_t i = 0; i < attribs.size(); ++i) {
            if(attribs[i] == a) {
                attribs.erase(attribs.begin() + i);
                break;
            }
        } 
    }

public:
    std::vector<T*>& getContainer() { return attribs; }
};

#endif
