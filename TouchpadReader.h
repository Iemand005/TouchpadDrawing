#pragma once

#include <Windows.h>

#include <hidsdi.h>
#pragma comment(lib, "hid.lib")

#include <string>

#include "Touch.h"

struct RAW_POSITION_PARTS {
    BYTE low;
    BYTE high;
};

struct RAW_TOUCH_POSITION {
    BYTE idAndEventType;
    RAW_POSITION_PARTS x, y;
};

struct RAW_TOUCH_SIZE {
    BYTE size;
    BYTE dimensions;
};

struct RAW_TOUCHPAD_EVENT {
    BYTE unk1;
    BYTE fingers;
    WORD time;

    RAW_TOUCH_POSITION positions[5];
    RAW_TOUCH_SIZE sizes[5];
};

struct TOUCHPAD_DATA {
    TOUCH touches[5];
    BYTE touchCount;
    DIMENSIONS touchpadSize;
};

class TouchpadReader
{
public:
	TouchpadReader() = default;
    TouchpadReader(HWND hWnd);
    ~TouchpadReader();
	bool RegisterWindow(HWND hWnd);
    bool RegisterRawInputDevice(HWND hWnd);
    PHIDP_PREPARSED_DATA GetPreparsedData(HANDLE hDevice);
    HIDP_CAPS GetDeviceCapabilities(HANDLE hDevice);
    HIDP_CAPS GetDeviceCapabilities(HANDLE hDevice, PHIDP_PREPARSED_DATA pPreparsedData);
    PHIDP_VALUE_CAPS GetDeviceCapabilityValues(HIDP_CAPS caps, PHIDP_PREPARSED_DATA pPreparsedData);
    DIMENSIONS GetTouchpadDimensions(HANDLE hDevice);
    BOOL IsTouchpadDevice(HANDLE hDevice);
    BOOL IsTouchpadDevice(HANDLE hDevice, PHIDP_PREPARSED_DATA pPreparsedData);
    TOUCHPAD_EVENT ProcessInput(HRAWINPUT hRawInput);
private:
    TOUCHPAD_EVENT touchpadData = { 0 };
};

