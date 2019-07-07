#ifndef SYS_TIMER_HPP
#define SYS_TIMER_HPP

#define NO_MIN_MAX
#include <windows.h>
#include <stdint.h>

class timer
{
public:
    timer()
    {
        QueryPerformanceFrequency(&_freq);
    }

    void start()
    {
        QueryPerformanceCounter(&_start);
    }
    
    float end()
    {
        QueryPerformanceCounter(&_end);
        _elapsed.QuadPart = _end.QuadPart - _start.QuadPart;
        _elapsed.QuadPart *= 1000000;
        _elapsed.QuadPart /= _freq.QuadPart;
        return _elapsed.QuadPart * .000001f;
    }
private:
    LARGE_INTEGER _freq;
    LARGE_INTEGER _start, _end;
    LARGE_INTEGER _elapsed;
};

#endif
