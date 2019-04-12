#ifndef DATA_READER_HPP
#define DATA_READER_HPP

#include "data_stream.hpp"

class DataReader {
public:
    DataReader(in_stream* strm)
    : strm(strm) {}
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool read(T& value) {
        strm->read(value);
        return true;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool read(std::vector<T>& value) {
        uint64_t sz = 0;
        strm->read(sz);
        strm->read(value, sz);
        return true;
    }
    bool read(std::string& value) {
        uint64_t sz = 0;
        strm->read(sz);
        strm->read(value, sz);
        return true;
    }

    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    T read() {
        T v;
        read(v);
        return v;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    std::vector<T> readArray() {
        std::vector<T> v;
        read(v);
        return v;
    }
    std::string readStr() {
        std::string v;
        read(v);
        return v;
    }
    
private:
    in_stream* strm;
};

#endif
