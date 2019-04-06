#ifndef LOG_HPP
#define LOG_HPP

#include <string>
#include <iostream>
#include <sstream>
#include <thread>
#include <queue>
#include <mutex>
#include <fstream>
#include <ctime>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "../gfxm.hpp"

#include "filesystem.hpp"

class Log {
public:
    enum Type {
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_DEBUG_INFO,
        LOG_DEBUG_WARN,
        LOG_DEBUG_ERROR
    };

    static Log* GetInstance() {
        static Log fl;
        return &fl;
    }
    static void Write(const std::string& str, Type type = LOG_INFO) {
        GetInstance()->_write(str, type);
    }
private:
    Log()
    : working(true) {
        thread_writer = std::thread([this](){
            tm ptm = {0};
            time_t t = time(0);
            localtime_s(&ptm, &t);
            char buffer[32];
            strftime(buffer, 32, "%d%m%Y", &ptm);

            createDirRecursive(get_module_dir() + "\\log");

            std::ofstream f(get_module_dir() + "\\log\\" + std::string(buffer) + ".log", std::ios::out | std::ios::app);

            do {
                std::queue<entry> lines_copy;
                {
                    std::lock_guard<std::mutex> lock(sync);
                    if(!working && lines.empty())
                        break;
                    
                    lines_copy = lines;
                    while(!lines.empty()) {
                        lines.pop();
                    }
                }
                while(!lines_copy.empty()) {
                    entry e = lines_copy.front();
                    tm ptm = {0};
                    localtime_s(&ptm, &e.t);
                    char buffer[32];
                    strftime(buffer, 32, "%H:%M:%S", &ptm); 
                    lines_copy.pop();

                    std::string str = static_cast<std::ostringstream&>(
                        std::ostringstream() << _typeToString(e.type) 
                        << " | " << buffer 
                        << " | " << std::hex << e.thread_id 
                        << ": " << e.line 
                        << std::endl).str();
                    f << str;
                    std::cout << str;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            } while(1);

            f << "\n\n\n";

            f.flush();
            f.close();
        });
    }
    ~Log() {
        working = false;
        if(thread_writer.joinable()) {
            thread_writer.join();
        }
    }

    void _write(const std::string& str, Type type) {
        std::lock_guard<std::mutex> lock(sync);
        lines.push(entry{
            type,
            time(0),
            GetCurrentThreadId(),
            str
        });
    }

    std::string _typeToString(Type type) {
        std::string str;
        switch(type) {
        case LOG_INFO:
            str = "INFO";
            break;
        case LOG_WARN:
            str = "WARN";
            break;
        case LOG_ERROR:
            str = "ERROR";
            break;
        case LOG_DEBUG_INFO:
            str = "D_INFO";
            break;
        case LOG_DEBUG_WARN:
            str = "D_WARN";
            break;
        case LOG_DEBUG_ERROR:
            str = "D_ERROR";
            break;
        }
        return str;
    }

    struct entry {
        Type type;
        time_t t;
        DWORD thread_id;
        std::string line;
    };

    bool working;
    std::mutex sync;
    std::queue<entry> lines;
    std::thread thread_writer;
};

#define MKSTR(LINE) \
static_cast<std::ostringstream&>(std::ostringstream() << LINE).str()

//#define LOG(LINE) std::cout << MKSTR(LINE) << std::endl;
#define LOG(LINE) Log::Write(MKSTR(LINE));
#define LOG_WARN(LINE) Log::Write(MKSTR(LINE), Log::LOG_WARN);
#define LOG_ERR(LINE) Log::Write(MKSTR(LINE), Log::LOG_ERROR);
#define LOG_DBG(LINE) Log::Write(MKSTR(LINE), Log::LOG_DEBUG_INFO);

inline std::ostream& operator<< (std::ostream& stream, const gfxm::vec2& v) {
    stream << "[" << v.x << ", " << v.y << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::vec3& v) {
    stream << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::vec4& v) {
    stream << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::quat& v) {
    stream << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::mat3& v) {
    stream << v[0] << "\n" 
        << v[1] << "\n"
        << v[2];
    return stream;
}
inline std::ostream& operator<< (std::ostream& stream, const gfxm::mat4& v) {
    stream << v[0] << "\n" 
        << v[1] << "\n"
        << v[2] << "\n"
        << v[3];
    return stream;
}

#endif
