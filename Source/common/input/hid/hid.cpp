#include "hid.hpp"


InputWin32Hid::~InputWin32Hid() {
    if(hidHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(hidHandle);
    }
}

bool InputWin32Hid::init(const char* device_path) {
    hidHandle = CreateFileA(device_path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    if (hidHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    if (HidD_GetPreparsedData(hidHandle, &pPreparsedData) == FALSE) {
        LOG_WARN("HidD_GetPreparsedData failed");
        return false;
    }
    NTSTATUS ret = HidP_GetCaps(pPreparsedData, &hidCaps);
    if (ret != HIDP_STATUS_SUCCESS) {
        LOG_WARN("HidP_GetCaps failed: " << ret);
        return false;
    }

    LOG("HID device found, usage: " << hidCaps.Usage);
    if(hidCaps.Usage == 0x04) {             // Joystick
        LOG("\tJoystick");
    } else if (hidCaps.Usage == 0x05) {     // Gamepad
        LOG("\tGamepad");
    } else {                                // Not interested
        return false;
    }

    if(HidD_GetAttributes(hidHandle, &hidAttributes) == FALSE) {
        LOG_WARN("Failed to get HID attributes");
        return false;
    }

    buttonCaps.resize(hidCaps.NumberInputButtonCaps);
    if (HidP_GetButtonCaps(HidP_Input, buttonCaps.data(), &hidCaps.NumberInputButtonCaps, pPreparsedData) != HIDP_STATUS_SUCCESS) {
        LOG_WARN("Failed to get HID input button caps");
        return false;
    }
    valueCaps.resize(hidCaps.NumberInputValueCaps);
    if (HidP_GetValueCaps(HidP_Input, valueCaps.data(), &hidCaps.NumberInputValueCaps, pPreparsedData) != HIDP_STATUS_SUCCESS) {
        LOG_WARN("Failed to get HID input value caps");
        return false;
    }

    outValueCaps.resize(hidCaps.NumberOutputValueCaps);
    if (HidP_GetValueCaps(HidP_Output, outValueCaps.data(), &hidCaps.NumberOutputValueCaps, pPreparsedData) != HIDP_STATUS_SUCCESS) {
        LOG_WARN("Failed to get HID output value caps");
        return false;
    }

    ULONG max_button_usage_list = 0;
    for(int i = 0; i < hidCaps.NumberInputButtonCaps; ++i) {
        ULONG count = HidP_MaxUsageListLength(HidP_Input, buttonCaps[i].UsagePage, pPreparsedData);
        if(max_button_usage_list < count) {
            max_button_usage_list = count;
        }
    }
    buttonUsageList.resize(max_button_usage_list);

    inputBuffer.resize(hidCaps.InputReportByteLength, 0);
    inputBufferPending.resize(hidCaps.InputReportByteLength, 0);

    outputBuffer.resize(hidCaps.OutputReportByteLength, 0);
    
    return true;
}

void InputWin32Hid::update() {
    if (hidHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    bool  hasNewData = false;
    if (isReadPending) {
        DWORD bytesTransferred;
        if (GetOverlappedResult(hidHandle, &overlappedRead, &bytesTransferred, FALSE) == FALSE) {
        if (GetLastError() == ERROR_IO_INCOMPLETE) {
            time_t curTime = time(0);
            if (curTime < readPendingStartTime || (curTime - readPendingStartTime) > 60) {
                CancelIoEx(hidHandle, &overlappedRead);
                isReadPending = false;
                return; // FALSE
            }

            return; // TRUE
        }
        } else {
        memcpy(inputBuffer.data(), inputBufferPending.data(), inputBufferPending.size());
        hasNewData = true;
        }
        isReadPending = false;
    }

    while (!isReadPending) {
        memset(&overlappedRead, 0, sizeof(overlappedRead));

        BOOL ret = ReadFile(hidHandle, inputBufferPending.data(), inputBufferPending.size(), NULL, &overlappedRead);
        if (ret == FALSE) {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING) {
            return; // FALSE
        }
        }

        DWORD bytesTransferred;
        if (GetOverlappedResult(hidHandle, &overlappedRead, &bytesTransferred, FALSE) == FALSE) {
        if (GetLastError() != ERROR_IO_INCOMPLETE) {
            return; // FALSE
        }
        readPendingStartTime = time(0);
        isReadPending = true;
        } else {
        memcpy(inputBuffer.data(), inputBufferPending.data(), inputBufferPending.size());
        hasNewData = true;
        }

    }

    if (!hasNewData) {
        return; // TRUE
    }

    ULONG usageListLen = buttonUsageList.size();
    for (int i = 0; i < hidCaps.NumberInputButtonCaps; ++i) {
        HidP_GetUsages(
            HidP_Input,
            buttonCaps[i].UsagePage,
            buttonCaps[i].LinkCollection,
            &buttonUsageList[0],
            &usageListLen,
            pPreparsedData,
            (PCHAR)&inputBuffer[0],
            inputBuffer.size()
        );

        // TODO: Support multiple pages

        if(usageListLen) {
            LOG("Page " << i);
        }
        for (ULONG j = 0; j < usageListLen; ++j) {
            USAGE u = buttonUsageList[j];
            LOG("Pressed " << (int)u);
        }
    }

    for (int i = 0; i < hidCaps.NumberInputValueCaps; ++i) {
        LOG("Page " << i);
        if(valueCaps[i].IsRange) {
            for(int u = valueCaps[i].Range.UsageMin; u < valueCaps[i].Range.UsageMax; ++u) {
                ULONG value = 0;
                float fvalue = .0f;
                HidP_GetUsageValue(
                    HidP_Input,
                    valueCaps[i].UsagePage,
                    valueCaps[i].LinkCollection,
                    u,
                    &value,
                    pPreparsedData,
                    (PCHAR)&inputBuffer[0],
                    inputBuffer.size()
                );

                fvalue = (float)value;
                //SetValue(cur_btn, fvalue);

                LOG("Value: " << fvalue);
            }
        } else {
            ULONG value = 0;
            float fvalue = .0f;
            HidP_GetUsageValue(
                HidP_Input,
                valueCaps[i].UsagePage,
                valueCaps[i].LinkCollection,
                valueCaps[i].NotRange.Usage,
                &value,
                pPreparsedData,
                (PCHAR)&inputBuffer[0],
                inputBuffer.size()
            );

            fvalue = (float)value;
            //SetValue(cur_btn, fvalue);

            LOG("Value: " << fvalue);
        }
    }

    
    // TODO: Output
    memset(outValueCaps.data(), 0, sizeof(outValueCaps[0]) * outValueCaps.size());
    for(int i = 0; i < outValueCaps.size(); ++i) {
        auto ret = HidP_SetUsageValue(
            HidP_Output,
            outValueCaps[i].UsagePage,
            outValueCaps[i].LinkCollection,
            outValueCaps[i].NotRange.Usage,
            32000,
            pPreparsedData,
            (char*)outputBuffer.data(),
            outputBuffer.size()
        );
        if(ret != HIDP_STATUS_SUCCESS) {
            LOG_WARN("HidP_SetUsageValue failed: " << ret);
        }
    }
    
    outputBuffer[0] = 0x05;
    outputBuffer[1] = 0xff;

    outputBuffer[2] = 0;
    outputBuffer[3] = 0;
    outputBuffer[4] = 255; //fast motor
    outputBuffer[5] = 255; //slow  motor
    outputBuffer[6] = 255; //red
    outputBuffer[7] = 255; //green
    outputBuffer[8] = 255; //blue
    outputBuffer[9] = 255; //flash on duration
    outputBuffer[10] = 255; //flash off duration

    memset(&overlappedWrite, 0, sizeof(overlappedWrite));
    if(WriteFile(hidHandle, outputBuffer.data(), outputBuffer.size(), 0, &overlappedWrite) == FALSE) {
        LOG_WARN("Failed to write to HID: " << GetLastError());
    }

    if(HidD_SetOutputReport(hidHandle, outputBuffer.data(), outputBuffer.size()) != TRUE) {
        LOG_WARN("HidD_SetOutputReport");
    }
}

#include <map>
#include <string>

std::map<std::string, InputWin32Hid*> hidevices;

bool hidEnumDevices () {
   GUID hidGUID;
   HidD_GetHidGuid(&hidGUID);

   HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&hidGUID, 0, 0, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);
   if (hDevInfoSet == INVALID_HANDLE_VALUE) {
      return false;
   }

   SP_DEVINFO_DATA deviceInfoData;
   deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

   SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
   deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

   // Enumerate through all devices in hDevInfoSet
   std::vector<int8_t> buffer;
   for (int i = 0; SetupDiEnumDeviceInfo(hDevInfoSet, i, &deviceInfoData); ++i) {
      for (int ifaceIdx = 0; SetupDiEnumDeviceInterfaces(hDevInfoSet, &deviceInfoData, &hidGUID, ifaceIdx, &deviceInterfaceData); ++ifaceIdx) {

         // get path to device
         DWORD bufferSize = 0;
         SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &deviceInterfaceData, 0, 0, &bufferSize, NULL);
         if (bufferSize == 0) {
            continue;
         }

         buffer.resize(bufferSize * 2 + sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA));

         SP_DEVICE_INTERFACE_DETAIL_DATA * pDeviceInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)buffer.data();
         pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

         if (SetupDiGetDeviceInterfaceDetail(hDevInfoSet, &deviceInterfaceData, pDeviceInterfaceDetailData, bufferSize, &bufferSize, NULL) == FALSE) {
            continue;
         }

         InputWin32Hid* hid = new InputWin32Hid;
         if(hid->init(pDeviceInterfaceDetailData->DevicePath)) {
             hidevices[pDeviceInterfaceDetailData->DevicePath] = hid;
         }
      }
   }

   SetupDiDestroyDeviceInfoList(hDevInfoSet);

   return true;
}

void hidUpdateDevices() {
    for(auto& kv : hidevices) {
        kv.second->update();
    }
}