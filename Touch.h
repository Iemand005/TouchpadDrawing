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