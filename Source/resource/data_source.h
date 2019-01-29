#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <string>
#include <memory>
#include <vector>
#include <fstream>

template<typename Byte = char>
class streambuf_view : public std::streambuf {
public:
    using byte = Byte;
    
    streambuf_view()
    : _begin(0), _end(0), _cursor(0) {}
    streambuf_view(byte* data, size_t sz)
    : _begin(data), _end(data + sz), _cursor(data) {}
    streambuf_view(byte* begin, byte* end)
    : _begin(begin), _end(end), _cursor(begin) {}

    streambuf_view& operator=(const streambuf_view& other) {
        _begin = other._begin;
        _end = other._end;
        _cursor = other._cursor;
        return *this;
    }
    
protected:
    int_type underflow() override {
        return (_cursor == _end ? traits_type::eof() : traits_type::to_int_type(*_cursor));
    }
    int_type uflow() override {
        return (_cursor == _end ? traits_type::eof() : traits_type::to_int_type(*_cursor++));
    }
    int_type pbackfail(int_type ch) override {
        if(_cursor == _begin || (ch != traits_type::eof() && ch != _cursor[-1])) {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*--_cursor);
    }
    std::streamsize showmanyc() override {
        return _end - _cursor;
    }
    pos_type seekpos(pos_type sp, std::ios_base::openmode which) override {
        return seekoff(sp - pos_type(off_type(0)), std::ios_base::beg, which);
    }
    pos_type seekoff(off_type off,
        std::ios_base::seekdir dir,
        std::ios_base::openmode which = std::ios_base::in
    ) override {
        if (dir == std::ios_base::cur)
            gbump(off);
        else if (dir == std::ios_base::end)
            setg(eback(), egptr() + off, egptr());
        else if (dir == std::ios_base::beg)
            setg(eback(), eback() + off, egptr());
        return gptr() - eback();
    }

    byte* _begin;
    byte* _end;
    byte* _cursor;
}; 

typedef streambuf_view<> charbuf_view;

class DataSource {
public:
    virtual                 ~DataSource() {}

    virtual bool            ReadAll(char* dest) = 0;
    virtual uint64_t        Size() const = 0;

    virtual std::istream&   open_stream() = 0;
    virtual void            close_stream() = 0;

    void                    SetName(const std::string& name) { this->name = name; }
    const                   std::string& Name() const { return name; }
private:
    std::string name;
};

#define MINIZ_HEADER_FILE_ONLY
#include "../lib/miniz.c"

class DataSourceArchive : public DataSource
{
public:
    DataSourceArchive(uint32_t file_index, std::shared_ptr<mz_zip_archive> archive)
    : file_index(file_index), archive(archive), istr(&buf_view) {}
    virtual bool ReadAll(char* dest) {
        mz_zip_archive_file_stat f_stat;
        mz_zip_reader_file_stat(archive.get(), file_index, &f_stat);
        mz_zip_reader_extract_file_to_mem(archive.get(), f_stat.m_filename, dest, (size_t)Size(), 0);
        return true;
    }
    virtual uint64_t Size() const {
        mz_zip_archive_file_stat f_stat;
        mz_zip_reader_file_stat(archive.get(), file_index, &f_stat);
        return f_stat.m_uncomp_size;
    }

    virtual std::istream&   open_stream() {
        mz_zip_archive_file_stat f_stat;
        mz_zip_reader_file_stat(archive.get(), file_index, &f_stat);
        data.resize(f_stat.m_uncomp_size);
        mz_zip_reader_extract_file_to_mem(archive.get(), f_stat.m_filename, (char*)data.data(), data.size(), 0);

        buf_view = charbuf_view(data.data(), data.size());
        return istr;
    }
    virtual void            close_stream() {
        buf_view = charbuf_view();
        data.clear();
    }
private:
    uint32_t file_index;
    std::shared_ptr<mz_zip_archive> archive;

    charbuf_view buf_view;
    std::istream istr;
    std::vector<char> data;
};

class DataSourceMemory : public DataSource
{
public:
    DataSourceMemory()
    : istr(&buf_view) {}
    DataSourceMemory(const char* src, size_t size)
    : istr(&buf_view) {
        Fill(src, size);
    }
    ~DataSourceMemory() {}

    void Fill(const char* src, size_t size) {
        data.resize(size);
        memcpy((void*)data.data(), src, size);
    }

    virtual bool ReadAll(char* dest) {
        memcpy(dest, data.data(), data.size());
        return true;
    }
    virtual uint64_t Size() const {
        return data.size();
    }

    virtual std::istream&   open_stream() {
        buf_view = charbuf_view(data.data(), data.size());
        return istr;
    }
    virtual void            close_stream() {}
private:
    charbuf_view buf_view;
    std::istream istr;
    std::vector<char> data;
};

class DataSourceFilesystem : public DataSource {
public:
    DataSourceFilesystem(const std::string& path)
    : path(path) {}
    virtual bool ReadAll(char* dest) {
        std::ifstream in(path, std::ifstream::binary);
        if(!in.is_open()) {
            return false;
        }
        in.seekg(0, std::ios::end);
        size_t sz = (size_t)in.tellg();
        in.seekg(0, std::ios::beg);
        in.read(dest, sz);
        return true; 
    }
    virtual uint64_t Size() const {
        std::ifstream in(path, std::ifstream::binary);
        in.seekg(0, std::ios::end);
        if(!in.is_open()) {
            return 0;
        }
        return in.tellg(); 
    }

    virtual std::istream&   open_stream() {
        f_in = std::ifstream(path, std::ifstream::binary);
        return f_in;
    }
    virtual void            close_stream() {
        f_in.close();
    }
private:
    std::string path;
    std::ifstream f_in;
};

typedef std::shared_ptr<DataSource> DataSourceRef;

#endif
