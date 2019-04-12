#ifndef ZIP_READER_HPP
#define ZIP_READER_HPP

#include <vector>

#define MINIZ_HEADER_FILE_ONLY
#include "../lib/miniz.c"
#include "log.hpp"

class ZipReader {
public:
    ZipReader(void* data, size_t sz) {
        memset(&archive, 0, sizeof(archive));
        if(!mz_zip_reader_init_mem(&archive, data, sz, 0)) {
            LOG_ERR("mz_zip_reader_init_mem failed");
            return;
        }
        valid = true;
    };
    ZipReader(const std::vector<char>& data)
    : ZipReader((void*)data.data(), data.size()) {

    }
    ZipReader(std::istream& in, size_t sz = 0)
    : ZipReader(to_buf(in, sz)) {

    }
    ~ZipReader() {
        if(valid) {
            mz_zip_reader_end(&archive);
        }
    }

    bool isValid() {
        return valid;
    }

    size_t fileCount() {
        if(!isValid()) return 0;
        return mz_zip_reader_get_num_files(&archive);
    }

    std::string getFileName(int loc) {
        if(!isValid()) return 0;
        mz_zip_archive_file_stat fstat;
        if(!mz_zip_reader_file_stat(&archive, loc, &fstat)) {
            LOG_WARN("ZipReader: failed to get file name at " << loc);
            return "";
        }
        return fstat.m_filename;
    }

    size_t getFileSize(const std::string& fname) {
        if(!isValid()) return 0;
        int loc = mz_zip_reader_locate_file(&archive, fname.c_str(), 0, 0);
        if(loc == -1) {
            LOG_WARN("ZipReader: failed to locate file " << fname);
            return 0;
        }
        return getFileSize(loc);
    }
    size_t getFileSize(int loc) {
        if(!isValid()) return 0;
        mz_zip_archive_file_stat fstat;
        if(!mz_zip_reader_file_stat(&archive, loc, &fstat)) {
            LOG_WARN("ZipReader: failed to get file size at " << loc);
            return 0;
        }
        return fstat.m_uncomp_size;
    }

    std::vector<char> extractFile(const std::string& fname) {
        if(!isValid()) return std::vector<char>();
        int loc = mz_zip_reader_locate_file(&archive, fname.c_str(), 0, 0);
        if(loc == -1) {
            LOG_WARN("ZipReader: failed to locate file " << fname);
            return std::vector<char>();
        }
        return extractFile(loc);
    }
    std::string extractString(const std::string& fname) {
        auto v = extractFile(fname);
        std::string s(v.data(), v.data() + v.size());
        return s;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    T extract(const std::string& fname) {
        T r = T();
        auto v = extractFile(fname);
        if(v.size() != sizeof(T)) {
            LOG_WARN("ZipReader type " << rttr::type::get<T>().get_name().to_string() << " and file " << fname << " sizes do not match");
            return r;
        }
        memcpy(&r, v.data(), sizeof(r));
        return r;
    }
    template<typename T, typename = typename std::enable_if<std::is_trivially_copyable<T>::value>::type>
    std::vector<T> extractArray(const std::string& fname) {
        std::vector<T> r;
        auto v = extractFile(fname);
        r.resize(v.size() / sizeof(T));
        memcpy(r.data(), v.data(), r.size() * sizeof(T));
        return r;
    }
    std::vector<char> extractFile(int loc) {
        if(!isValid()) return std::vector<char>();
        std::vector<char> fbuf(getFileSize(loc));
        if(fbuf.size() == 0) { return fbuf; }
        if(!mz_zip_reader_extract_to_mem(&archive, loc, (void*)fbuf.data(), fbuf.size(), 0)) {
            LOG_WARN("ZipReader: failed to extract file " << loc);
            return fbuf;
        }
        return fbuf;
    }
    
private:
    mz_zip_archive archive;
    bool valid = false;

    std::vector<char> to_buf(std::istream& in, size_t sz) {
        std::vector<char> buf(sz);
        in.read(buf.data(), sz);
        return buf;
    }
};

#endif
