#ifndef WIN_HUMAN_INTERFACE_DEVICE_HPP
#define WIN_HUMAN_INTERFACE_DEVICE_HPP

#define NO_MIN_MAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <hidsdi.h>
#include <setupapi.h>

#include "../../util/log.hpp"

class InputWin32Hid {
    HANDLE                  hidHandle = INVALID_HANDLE_VALUE;
    PHIDP_PREPARSED_DATA    pPreparsedData = 0;
    HIDD_ATTRIBUTES         hidAttributes = { 0 };
    HIDP_CAPS               hidCaps = { 0 };

    bool                    isReadPending = false;
    OVERLAPPED              overlappedRead;
    OVERLAPPED              overlappedWrite;
    time_t                  readPendingStartTime = 0;

    std::vector<HIDP_BUTTON_CAPS> buttonCaps;
    std::vector<HIDP_VALUE_CAPS>  valueCaps;
    std::vector<USAGE>            buttonUsageList;

    std::vector<HIDP_VALUE_CAPS>  outValueCaps;

    std::vector<int8_t>     inputBuffer;
    std::vector<int8_t>     inputBufferPending;
    std::vector<int8_t>     outputBuffer;

public:
    ~InputWin32Hid();

    bool init(const char* device_path);
    void update();
};

bool hidEnumDevices();
void hidUpdateDevices();


#endif
