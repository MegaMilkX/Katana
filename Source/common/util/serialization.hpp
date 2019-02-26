#ifndef UTIL_SERIALIZATION_HPP
#define UTIL_SERIALIZATION_HPP

#include <istream>
#include <ostream>

template<typename T>
void write(std::ostream& out, const T& value) {
    out.write((char*)&value, sizeof(T));
}
template<typename T>
T read(std::istream& in) {
    T r;
    in.read((char*)&r, sizeof(T));
    return r;
}

inline void wt_string(std::ostream& out, const std::string& str) {
    uint64_t sz = str.size();
    out.write((char*)&sz, sizeof(sz));
    if(sz) {
        out.write((char*)str.data(), sz);
    }
}
inline std::string rd_string(std::istream& in) {
    std::string r;
    uint64_t sz = 0;
    in.read((char*)&sz, sizeof(sz));
    r.resize(sz);
    if(sz) {
        in.read((char*)r.data(), sz);
    }
    return r;
}

#endif
