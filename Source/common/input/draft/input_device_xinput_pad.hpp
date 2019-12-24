#ifndef INPUT_DEVICE_XINPUT_PAD_HPP
#define INPUT_DEVICE_XINPUT_PAD_HPP


#include "input_device_tpl.hpp"
#include "adapters/input_adapter_xbox_pad.hpp"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#pragma comment (lib, "xinput.lib")

class InputDeviceXInputPad : public InputDeviceTpl<InputAdapterXboxPad> {
    XINPUT_STATE xinput_state;
    DWORD xinput_index;
public:
    InputDeviceXInputPad(DWORD xinput_index)
    : xinput_index(xinput_index) {}

    void update (void) {
        if(!getAdapter()) {
            return;
        }
        
        XINPUT_STATE state = { 0 };
        DWORD ret = XInputGetState(xinput_index, &state);
        if(ret != ERROR_SUCCESS) {
            // TODO: Device not connected
            return;
        }

        const uint16_t max_val = -1;
        getAdapter()->setKeyState(0, state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP ? max_val : 0);
        getAdapter()->setKeyState(1, state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN ? max_val : 0);
        getAdapter()->setKeyState(2, state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT ? max_val : 0);
        getAdapter()->setKeyState(3, state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ? max_val : 0);
        getAdapter()->setKeyState(4, state.Gamepad.wButtons & XINPUT_GAMEPAD_START ? max_val : 0);
        getAdapter()->setKeyState(5, state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK ? max_val : 0);
        getAdapter()->setKeyState(6, state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB ? max_val : 0);
        getAdapter()->setKeyState(7, state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ? max_val : 0);
        getAdapter()->setKeyState(8, state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ? max_val : 0);
        getAdapter()->setKeyState(9, state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ? max_val : 0);
        getAdapter()->setKeyState(10, state.Gamepad.wButtons & XINPUT_GAMEPAD_A ? max_val : 0);
        getAdapter()->setKeyState(11, state.Gamepad.wButtons & XINPUT_GAMEPAD_B ? max_val : 0);
        getAdapter()->setKeyState(12, state.Gamepad.wButtons & XINPUT_GAMEPAD_X ? max_val : 0);
        getAdapter()->setKeyState(13, state.Gamepad.wButtons & XINPUT_GAMEPAD_Y ? max_val : 0);

        getAdapter()->setKeyState(14, state.Gamepad.bLeftTrigger / 255.0f * max_val);
        getAdapter()->setKeyState(15, state.Gamepad.bRightTrigger / 255.0f * max_val);

        getAdapter()->setKeyState(16, state.Gamepad.sThumbLX);
        getAdapter()->setKeyState(17, state.Gamepad.sThumbLY);

        getAdapter()->setKeyState(18, state.Gamepad.sThumbRX);
        getAdapter()->setKeyState(19, state.Gamepad.sThumbRY);
    }
};


#endif
