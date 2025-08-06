#pragma once

#include <Windows.h>

struct DIMENSIONS {
    UINT width;
    UINT height;
};

struct TOUCH {
    BYTE id;
    POINT position;
    DIMENSIONS dimensions;
    BYTE size;
    BOOL down;
};

struct TOUCH_EVENT {
    TOUCH touch;
    BYTE eventType; // 3 for update, 1 for up
};

struct TOUCHPAD_EVENT {
    TOUCH_EVENT touches[5];
    BYTE touchCount;
    DIMENSIONS touchpadSize;
	BOOL buttonPressed;
};