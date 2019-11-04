#ifndef TIMER_HPP___
#define TIMER_HPP___


#define NOMINMAX
#include <windows.h>
#include <stdint.h>

class timer
{
public:
    timer()
    {
        QueryPerformanceFrequency(&freq);
    }

    void start()
    {
        QueryPerformanceCounter(&_start);
    }
    
    int64_t stop()
    {
        QueryPerformanceCounter(&_end);
        elapsed.QuadPart = _end.QuadPart - _start.QuadPart;
        elapsed.QuadPart *= 1000000;
        elapsed.QuadPart /= freq.QuadPart;
        return elapsed.QuadPart;
    }
private:
    LARGE_INTEGER freq;
    LARGE_INTEGER _start, _end;
    LARGE_INTEGER elapsed;
};


#endif
