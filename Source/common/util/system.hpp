#ifndef UTIL_SYSTEM_HPP
#define UTIL_SYSTEM_HPP

#include <string>
#include <algorithm>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool create_process(const std::string& fname, const std::string& args) {
    PROCESS_INFORMATION proc_info;
    STARTUPINFOA startup_info;
    ZeroMemory(&startup_info, sizeof(startup_info));
    startup_info.cb = sizeof(startup_info);
    char buf[256];
    memset(buf, 0, sizeof(buf));
    memcpy(buf, args.c_str(), std::min(args.size(), sizeof(buf)));
    BOOL r = CreateProcessA(
        fname.c_str(), 
        buf,
        NULL, NULL, FALSE, 0, NULL, NULL,
        &startup_info, &proc_info
    );

    return r == TRUE;
}

#endif
