#include "TouchpadReader.h"

bool TouchpadReader::GetPreparsedData(HANDLE hDevice, PHIDP_PREPARSED_DATA& pPreparsedData) {
    UINT preparsedSize = 0;
    if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &preparsedSize) == 0)
        return false;
    pPreparsedData = (PHIDP_PREPARSED_DATA)malloc(preparsedSize);
    if (!pPreparsedData)
        return false;
    if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &preparsedSize) == 0)
    {
        free(pPreparsedData);
        return false;
    }
    return true;
}

bool TouchpadReader::GetDeviceCapabilities(HANDLE hDevice, HIDP_CAPS& caps) {

}

bool TouchpadReader::IsTouchpadDevice(HANDLE hDevice)
{
    bool isTouchpad = false;
    UINT preparsedSize = 0;
    GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &preparsedSize);
    
    PHIDP_PREPARSED_DATA pPreparsed = (PHIDP_PREPARSED_DATA)malloc(preparsedSize);

    if (pPreparsed && GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, pPreparsed, &preparsedSize) != 0)
    {
        HIDP_CAPS caps;
        if (HidP_GetCaps(pPreparsed, &caps) == HIDP_STATUS_SUCCESS)
            isTouchpad = (caps.UsagePage == 0x0D && caps.Usage == 0x05);
    }
    free(pPreparsed);
    return isTouchpad;
}

bool TouchpadReader::GetTouchpadDimensions(HANDLE hDevice, TOUCHPAD_SIZE& size)
{
    // 1. Get preparsed data
    PHIDP_PREPARSED_DATA pPreparsed = NULL;
    UINT preparsedSize = 0;

    if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &preparsedSize) != 0)
        return false;

    pPreparsed = (PHIDP_PREPARSED_DATA)malloc(preparsedSize);
    if (!pPreparsed) return false;

    if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, pPreparsed, &preparsedSize) == 0)
    {
        free(pPreparsed);
        return false;
    }

    // 2. Get device capabilities
    HIDP_CAPS caps;
    if (HidP_GetCaps(pPreparsed, &caps) != HIDP_STATUS_SUCCESS)
    {
        free(pPreparsed);
        return false;
    }

    // 3. Get value capabilities for X/Y axes
    PHIDP_VALUE_CAPS pValueCaps = (PHIDP_VALUE_CAPS)malloc(caps.NumberInputValueCaps * sizeof(HIDP_VALUE_CAPS));
    if (!pValueCaps)
    {
        free(pPreparsed);
        return false;
    }

    USHORT valueCapsLength = caps.NumberInputValueCaps;
    if (HidP_GetValueCaps(HidP_Input, pValueCaps, &valueCapsLength, pPreparsed) != HIDP_STATUS_SUCCESS)
    {
        free(pValueCaps);
        free(pPreparsed);
        return false;
    }

    // 4. Find X and Y axes
    LONG xMin = 0, xMax = 0;
    LONG yMin = 0, yMax = 0;

    for (USHORT i = 0; i < valueCapsLength; i++)
    {
        if (pValueCaps[i].UsagePage == 0x01)  // Generic Desktop Page
        {
            if (pValueCaps[i].Range.UsageMin == 0x30) // X axis
            {
                xMin = pValueCaps[i].LogicalMin;
                xMax = pValueCaps[i].LogicalMax;
            }
            else if (pValueCaps[i].Range.UsageMin == 0x31) // Y axis
            {
                yMin = pValueCaps[i].LogicalMin;
                yMax = pValueCaps[i].LogicalMax;
            }
        }
    }

    size.width = xMax - xMin;
    size.height = yMax - yMin;

    free(pValueCaps);
    free(pPreparsed);
    return true;
}

bool TouchpadReader::ProcessInput(HRAWINPUT hRawInput)
{

    UINT size = 0;

    GetRawInputData(hRawInput, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER)) == 0;

    BYTE* buffer = new BYTE[size];
    if (GetRawInputData(hRawInput, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size)
    {
        RAWINPUT* raw = (RAWINPUT*)buffer;

        if (raw->header.dwType == RIM_TYPEHID)
        {
            if (IsTouchpadDevice(raw->header.hDevice))
            {
                TOUCHPAD_EVENT* touch = (TOUCHPAD_EVENT*)(raw->data.hid.bRawData);

                touchCount = touch->fingers >> 4;

                for (size_t i = 0; i < touchCount; i++)
                {
                    TOUCH_POSITION position = touch->positions[i];
                    UINT x = (((WORD)position.x.high) << 8) + position.x.low;
                    UINT y = (((WORD)position.y.high) << 8) + position.y.low;


                    TOUCH_SIZE size = touch->sizes[i];
                    int width = size.dimensions >> 4;
                    int height = size.dimensions & 0b00001111;

                    touches[i].rect.left = x;
                    touches[i].rect.top = y;
                    touches[i].rect.right = width;
                    touches[i].rect.bottom = height;
                    touches[i].size = size.size;
                }

                return true;
            }
            else OutputDebugString(L"Ignoring non-touchpad HID input\n");
        }
    }
    delete[] buffer;

    return false;
}