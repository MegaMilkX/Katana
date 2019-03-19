#ifndef ZIP_WRITER_HPP
#define ZIP_WRITER_HPP

#include <vector>
#include <sstream>

#define MINIZ_HEADER_FILE_ONLY
#include "../lib/miniz.c"
#include "log.hpp"

class ZipWriter {
public:
    ZipWriter() {
        memset(&archive, 0, sizeof(archive));
        if(!mz_zip_writer_init_heap(&archive, 0, 65537)) {
            LOG("ZipWriter: Failed to create archive file in memory");
            return;
        }
        valid = true;
    }
    ~ZipWriter() {
        if(valid) {
            mz_zip_writer_end(&archive);
        }
    }

    bool isValid() const { return valid; }

    void add(const std::string& fname, void* data, size_t sz) {
        if(!mz_zip_writer_add_mem(
            &archive, 
            fname.c_str(), 
            data, 
            sz, 
            0
        )){
            LOG_WARN("Failed to mz_zip_writer_add_mem() ");
        }
    }
    void add(const std::string& fname, const std::vector<char>& buf) {
        add(fname, (void*)buf.data(), buf.size());
    }
    void add(const std::string& fname, std::istream& in) {
        in.seekg(0, std::ios::end);
        size_t sz = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<char> buf;
        buf.resize(sz);
        in.read((char*)buf.data(), sz);
        
        add(fname, buf);
    }
    void add(const std::string& fname, const std::string& val) {
        add(fname, (void*)val.data(), val.size());
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    void add(const std::string& fname, const T& val) {
        add(fname, (void*)&val, sizeof(val));
    }
    template<typename T>
    void add(const std::string& fname, const std::vector<T>& array) {
        add(fname, (void*)array.data(), array.size() * sizeof(T));
    }

    std::vector<char> finalize() {
        std::vector<char> buf;
        void* archbuf = 0;
        size_t sz = 0;
        mz_zip_writer_finalize_heap_archive(&archive, &archbuf, &sz);
        buf = std::vector<char>((char*)archbuf, (char*)archbuf + sz);
        if(archbuf == 0 || sz == 0) {
            LOG_WARN("Finalized zip data is zero");
        }
        return buf;
    }
private:
    mz_zip_archive archive;
    bool valid = false;
};

#endif
