#ifndef DATA_WRITER_HPP
#define DATA_WRITER_HPP

#include "data_stream.hpp"

class DataWriter {
public:
    DataWriter(out_stream* strm)
    : strm(strm) {}
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const T& value) {
        strm->write(value);
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const std::vector<T>& value) {
        strm->write<uint64_t>((uint64_t)value.size());
        strm->write(value);
    }
    void write(const std::string& value) {
        strm->write<uint64_t>((uint64_t)value.size());
        strm->write(value);
    }
private:
    out_stream* strm;
};

#endif
