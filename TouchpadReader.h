#pragma once

#include <Windows.h>

#include <hidsdi.h>
#pragma comment(lib, "hid.lib")

struct POSITION_PARTS {
    BYTE low;
    BYTE high;
};

struct TOUCH_POSITION {
    BYTE index;
    POSITION_PARTS x, y;
};

struct TOUCH_SIZE {
    BYTE size;
    BYTE dimensions;
};

struct TOUCHPAD_SIZE {
	UINT width;
	UINT height;
};

struct TOUCHPAD_EVENT {
    BYTE unk1;
    BYTE fingers;
    WORD time;

    TOUCH_POSITION positions[5];
    TOUCH_SIZE sizes[5];
};

struct TOUCH {
    RECT rect;
    BYTE size;
};

class TouchpadReader
{
public:
    bool GetPreparsedData(HANDLE hDevice, PHIDP_PREPARSED_DATA& pPreparsedData);
    bool ProcessInput(HRAWINPUT rawInput);
    bool IsTouchpadDevice(HANDLE hDevice);
private:
    TOUCH touches[5];
    BYTE touchCount = 0;
    POINT touchpadSize;
};

