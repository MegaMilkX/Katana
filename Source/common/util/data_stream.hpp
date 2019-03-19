#ifndef DATA_STREAM_HPP
#define DATA_STREAM_HPP

#include <type_traits>
#include <string>
#include <vector>
#include <sstream>

class data_stream {
public:
    template<typename T>
    void write(const T& value);
};

class in_stream {
public:
    bool read(void* data, size_t sz) {
        if(buf.size() - cur < sz) {
            return false;
        }
        memcpy(data, buf.data() + cur, sz);
        cur += sz;
        return true;
    }

    bool eof() {
        return buf.size() == cur;
    }
private:
    std::vector<char> buf;
    size_t cur = 0;
};

class out_stream {
public:
    void write(const void* data, size_t sz) {
        buf.insert(buf.end(), (char*)data, (char*)data + sz);
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const T& value) {
        write(&value, sizeof(value));
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void write(const std::vector<T>& value) {
        write(value.data(), sizeof(T) * value.size());
    }
    void write(const std::string& value) {
        write(value.data(), value.size());
    }

    std::vector<char>& getBuffer() {
        return buf;
    }
private:
    std::vector<char> buf;
};

#endif
