#ifndef STATIC_RUN_H
#define STATIC_RUN_H

#define STATIC_RUN(UNIQUE_NAME) \
template<typename T> \
struct UNIQUE_NAME ## _dummyWrap { \
    static int dummy; \
}; \
inline int UNIQUE_NAME ## _dummyUsage() { \
    return UNIQUE_NAME ## _dummyWrap<int>::dummy; \
} \
inline int UNIQUE_NAME ## _staticRunFunc(); \
template<typename T> \
int UNIQUE_NAME ## _dummyWrap<T>::dummy = UNIQUE_NAME ## _staticRunFunc(); \
inline void UNIQUE_NAME ## _staticRunFuncImpl(); \
inline int UNIQUE_NAME ## _staticRunFunc() { UNIQUE_NAME ## _staticRunFuncImpl(); return 0; } \
inline void UNIQUE_NAME ## _staticRunFuncImpl()

#endif
