#ifndef DATA_STREAM_HPP
#define DATA_STREAM_HPP

class data_stream {
public:
    template<typename T>
    void write(const T& value);
    template<typename T>
};

class in_stream {
public:
    template<typename T>
    T read();
};

#endif
