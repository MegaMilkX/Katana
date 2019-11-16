#ifndef ANIM_BLACKBOARD_HPP
#define ANIM_BLACKBOARD_HPP

#include <map>
#include <string>


class AnimBlackboard {
    std::map<std::string, float> values;
public:
    void set(const char* name, float val) {
        values[name] = val;
    }
    void set(const char* name, int val) {
        values[name] = val;
    }
    void set(const char* name, bool val) {
        if(val) {
            values[name] = true;
        } else {
            values[name] = false;
        }
    }

    float get_float(const char* name) {
        return values[name];
    }
    int get_int(const char* name) {
        return (int)values[name];
    }
    bool get_bool(const char* name) {
        return ((int)values[name]) != 0;
    }
};


#endif
