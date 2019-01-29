#ifndef CURVE_H
#define CURVE_H

#include "../../gfxm.hpp"
#include <vector>

template<typename T>
struct keyframe
{
    keyframe(): time(0) {}
    keyframe(float time): time(time) {}
    void operator=(const T& value)
    { this->value = value; }
    bool operator<(const keyframe<T>& other)
    { return time < other.time; }
    float time;
    T value;
};

template<typename T>
T curve_value_interp(const T& a, const T& b, float t) {
    return T();
}
template<>
inline float curve_value_interp(const float& a, const float& b, float t) {
    return a + t * (b - a);
}
template<>
inline gfxm::vec2 curve_value_interp(const gfxm::vec2& a, const gfxm::vec2& b, float t) {
    return gfxm::vec2(
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y)
    );
}
template<>
inline gfxm::vec3 curve_value_interp(const gfxm::vec3& a, const gfxm::vec3& b, float t) {
    return gfxm::vec3(
        a.x + t * (b.x - a.x),
        a.y + t * (b.y - a.y),
        a.z + t * (b.z - a.z)
    );
}
template<>
inline gfxm::quat curve_value_interp(const gfxm::quat& a, const gfxm::quat& b, float t) {
    return gfxm::slerp(a, b, t);
}

template<typename T>
T curve_value_delta(const T& a, const T& b) {
    return b - a;
}
template<>
inline float curve_value_delta(const float& a, const float& b) {
    return b - a;
}
template<>
inline gfxm::vec2 curve_value_delta(const gfxm::vec2& a, const gfxm::vec2& b) {
    return b - a;
}
template<>
inline gfxm::vec3 curve_value_delta(const gfxm::vec3& a, const gfxm::vec3& b) {
    return b - a;
}
template<>
inline gfxm::quat curve_value_delta(const gfxm::quat& a, const gfxm::quat& b) {
    return b * gfxm::inverse(a);
}

template<typename T>
T curve_value_add(const T& a, const T& b) {
    return a + b;
}
template<>
inline gfxm::quat curve_value_add(const gfxm::quat& a, const gfxm::quat& b) {
    return b * a;
}

template<typename T>
class curve {
public:
    typedef keyframe<T> keyframe_t;

    T at(float time, T def = T()) {
        if(keyframes.empty())
            return def;
        keyframe<T>* k0 = 0, *k1 = 0;
        for(int i = keyframes.size() - 1; i >= 0; --i)
        {
            k0 = &keyframes[i];
            if(i == keyframes.size() - 1)
                k1 = k0;
            else
                k1 = &keyframes[i + 1];
            if(k0->time <= time)
                break;
        }
        if(k0 == 0)
            return def;
        if(k0 == k1)
            return k0->value;
        T a = k0->value;
        T b = k1->value;
        float t = (time - k0->time) / (k1->time - k0->time);
        return curve_value_interp(a, b, t);
    }
    T delta(float from, float to) {
        if(from < to) {
            T at0 = at(from);
            T at1 = at(to);
            return curve_value_delta(at0, at1);
        } else if(from > to) {
            T at_from0 = at(from);
            T at_from1 = at(keyframes[keyframes.size() - 1].time);
            T at_to0 = at(keyframes[0].time);
            T at_to1 = at(to);
            return curve_value_add(
                curve_value_delta(at_from0, at_from1),
                curve_value_delta(at_to0, at_to1)
            );
        } else {
            return T();
        }
    }
    keyframe<T>& operator[](float time) {
        for(unsigned i = 0; i < keyframes.size(); ++i)
        {
            if(fabsf(keyframes[i].time - time) < FLT_EPSILON)
            {
                return keyframes[i];
            }
        }        
        keyframes.push_back(keyframe<T>(time));
        std::sort(keyframes.begin(), keyframes.end());
        return operator[](time);
    }
    void set_keyframes(const std::vector<keyframe<T>> keyframes) {
        this->keyframes = keyframes;
        std::sort(this->keyframes.begin(), this->keyframes.end());
    }
    std::vector<keyframe<T>>& get_keyframes() {
        return keyframes;
    }
private:
    std::vector<keyframe<T>> keyframes;
};

/*
class curve
{
public:
    float at(float time, float def = 0.0f)
    {
        if(keyframes.empty())
            return def;
        keyframe<float>* k0 = 0, *k1 = 0;
        for(int i = keyframes.size() - 1; i >= 0; --i)
        {
            k0 = &keyframes[i];
            if(i == keyframes.size() - 1)
                k1 = k0;
            else
                k1 = &keyframes[i + 1];
            if(k0->time <= time)
                break;
        }
        if(k0 == 0)
            return def;
        if(k0 == k1)
            return k0->value;
        float a = k0->value;
        float b = k1->value;
        float t = (time - k0->time) / (k1->time - k0->time);
        return a + t * (b - a);
    }

    float delta(float from, float to)
    {
        if(keyframes.empty())
            return 0.0f;
        float d = 0.0f;
        float prevValue = at(from);
        float value = at(to);

        if(to < from)
        {
            float d0 = keyframes.back().value - prevValue;
            float d1 = value - keyframes.front().value;
            d = d0 + d1;
        }
        else
        {
            d = value - prevValue;
        }
        return d;
    }

    keyframe<float>& operator[](float time)
    {
        for(unsigned i = 0; i < keyframes.size(); ++i)
        {
            if(fabsf(keyframes[i].time - time) < FLT_EPSILON)
            {
                return keyframes[i];
            }
        }
        
        keyframes.push_back(keyframe<float>(time));
        std::sort(keyframes.begin(), keyframes.end());
        return operator[](time);
    }

    bool empty() { return keyframes.empty(); }
private:
    std::vector<keyframe<float>> keyframes;
};

struct curve2
{
    gfxm::vec2 at(float t, const gfxm::vec2& def)
    {
        return gfxm::vec2(x.at(t, def.x), y.at(t, def.y));
    }
    gfxm::vec2 delta(float from, float to)
    {
        return gfxm::vec2(
            x.delta(from, to), 
            y.delta(from, to)
        );
    }
    bool empty() { return x.empty() || y.empty(); }
    curve x, y;
};

struct curve3
{
    gfxm::vec3 at(float t, const gfxm::vec3& def)
    {
        return gfxm::vec3(
            x.at(t, def.x), 
            y.at(t, def.y), 
            z.at(t, def.z));
    }
    gfxm::vec3 delta(float from, float to)
    {
        return gfxm::vec3(
            x.delta(from, to),
            y.delta(from, to),
            z.delta(from, to)
        );
    }
    bool empty() { return x.empty() || y.empty() || x.empty(); }
    curve x, y, z;
};

struct curve4
{
    gfxm::vec4 at(float t, const gfxm::vec4& def)
    {
        return gfxm::vec4(
            x.at(t, def.x), 
            y.at(t, def.y), 
            z.at(t, def.z), 
            w.at(t, def.w));
    }
    gfxm::vec4 delta(float from, float to)
    {
        return gfxm::vec4(
            x.delta(from, to),
            y.delta(from, to),
            z.delta(from, to),
            w.delta(from, to)
        );
    }
    bool empty() { return x.empty() || y.empty() || x.empty() || w.empty(); }
    curve x, y, z, w;
};

struct curveq
{
public:
    gfxm::quat at(float t, const gfxm::quat& def)
    {
        return gfxm::normalize(
            gfxm::quat(
                x.at(t, def.x), 
                y.at(t, def.y), 
                z.at(t, def.z), 
                w.at(t, def.w)
            )
        );
    }
    bool empty() { return x.empty() || y.empty() || x.empty() || w.empty(); }
    curve x, y, z, w;
};
*/
#endif
