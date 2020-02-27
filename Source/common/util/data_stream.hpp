#ifndef DATA_STREAM_HPP
#define DATA_STREAM_HPP

#include <type_traits>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

class in_stream {
public:
    virtual ~in_stream() {}
    virtual bool read(void* data, size_t sz) = 0;
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool read(T& value) {
        return read(&value, sizeof(value));
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    bool read(std::vector<T>& value, size_t count) {
        value.resize(count);
        return read((void*)value.data(), count * sizeof(T));
    }
    bool read(std::string& value, size_t count) {
        value.resize(count);
        return read((void*)value.data(), count);
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    T read() {
        T v;
        read(v);
        return v;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    std::vector<T> readArray(size_t sz) {
        std::vector<T> v;
        read(v, sz);
        return v;
    }
    std::string readStr(size_t sz) {
        std::string v;
        read(v, sz);
        return v;
    }

    virtual size_t bytes_available() = 0;
};

class out_stream {
public:
    virtual ~out_stream() {}
    virtual void write(const void* data, size_t sz) = 0;
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
};

class base_stream : public in_stream, public out_stream {
public:
    virtual ~base_stream() {}

    virtual void seek(size_t sz) = 0;
    virtual void jump(size_t pos) = 0;

    virtual bool eof() = 0;

    virtual size_t size() = 0;
protected:
    std::vector<char> buf;
    size_t cur = 0;
};

class dstream : public base_stream {
public:
    virtual void seek(size_t sz);
    virtual void jump(size_t pos);

    virtual bool eof();

    virtual size_t size();

    void setBuffer(const std::vector<char>& buf);
    std::vector<char>& getBuffer();

    virtual bool read(void* data, size_t sz);
    
    virtual void write(const void* data, size_t sz);

    virtual size_t bytes_available();
    
private:
    std::vector<char> buf;
    size_t cur = 0;
};

class file_stream : public base_stream {
public:
    enum MODE {
        F_IN = 1,
        F_OUT = 2
    };
    file_stream(const std::string& path, MODE m = F_IN);

    bool is_open();

    virtual void seek(size_t sz);
    virtual void jump(size_t pos);
    virtual bool eof();
    virtual size_t size();

    virtual bool read(void* data, size_t sz);
    virtual void write(const void* data, size_t sz);

    virtual size_t bytes_available();
private:
    std::fstream f;
    size_t sz = 0;
};

#endif
