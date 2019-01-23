#ifndef UTIL_WIN32_MODULE_HPP
#define UTIL_WIN32_MODULE_HPP

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

inline HMODULE this_module_handle()
{
  HMODULE h = NULL;
  GetModuleHandleExW(
    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
    reinterpret_cast<LPCWSTR>(&this_module_handle),
    &h
  );
  return h;
}

#endif
