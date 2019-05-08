#include "data_stream.hpp"

#include <windows.h>

#include "log.hpp"

void dstream::seek(size_t sz) {
    if(cur <= buf.size())
        cur+=sz;
}
void dstream::jump(size_t pos) {
    cur = pos;
}

bool dstream::eof() {
    return buf.size() < cur;
}

size_t dstream::size() {
    return buf.size();
}

void dstream::setBuffer(const std::vector<char>& buf) {
    this->buf = buf;
    cur = 0;
}
std::vector<char>& dstream::getBuffer() {
    return buf;
}

bool dstream::read(void* data, size_t sz) {
    if(buf.size() - cur < sz) {
        return false;
    }
    memcpy(data, buf.data() + cur, sz);
    seek(sz);
    return true;
}

void dstream::write(const void* data, size_t sz) {
    buf.insert(buf.end(), (char*)data, (char*)data + sz);
    jump(buf.size());
    //seek(sz);
}

size_t dstream::bytes_available() {
    return size() - cur;
}

file_stream::file_stream(const std::string& path, MODE m) {
    int mode_ = std::fstream::binary;
    if(m & MODE::F_IN) {
        mode_ |= std::fstream::in;
    }
    if(m & MODE::F_OUT) {
        mode_ |= std::fstream::out;
    }

    f = std::fstream(path, mode_);
    if(f.is_open()) {
        f.seekg(0, std::ios::end);
        sz = (size_t)f.tellg();
        f.seekg(0, std::ios::beg);
    } else {
        LOG_WARN(std::strerror(errno) << ": " << path);
    }
}

bool file_stream::is_open() {
    return f.is_open();
}

void file_stream::seek(size_t sz) {
    if(!f.is_open()) return;
    f.seekg(sz);
}
void file_stream::jump(size_t pos) {
    if(!f.is_open()) return;
    f.seekg(pos, std::ios::beg);
}
bool file_stream::eof() {
    if(!f.is_open()) return true;
    return f.eof();
}
size_t file_stream::size() {
    return sz;
}

bool file_stream::read(void* data, size_t sz) {
    if(!f.is_open()) return false;
    f.read((char*)data, sz);
    cur+=sz;
    return true;
}
void file_stream::write(const void* data, size_t sz) {
    if(!f.is_open()) return;
    f.write((char*)data, sz);
    this->sz += sz;
    cur = this->sz;
}

size_t file_stream::bytes_available() {
    return size() - cur;
}