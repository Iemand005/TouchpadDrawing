#include "TouchpadReader.h"

TouchpadReader::TouchpadReader(HWND hWnd){
	RegisterWindow(hWnd);
}

TouchpadReader::~TouchpadReader()
{
}

bool TouchpadReader::RegisterWindow(HWND hWnd) {
    return RegisterTouchWindow(hWnd, 0);
}

PHIDP_PREPARSED_DATA TouchpadReader::GetPreparsedData(HANDLE hDevice) {
    UINT preparsedSize = 0;
    if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &preparsedSize) == 0) {
        PHIDP_PREPARSED_DATA pPreparsedData = (PHIDP_PREPARSED_DATA)malloc(preparsedSize);
        if (!pPreparsedData)
            return false;
        if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &preparsedSize) != 0)
            return pPreparsedData;
        
        free(pPreparsedData);
    }
    return NULL;
}

HIDP_CAPS TouchpadReader::GetDeviceCapabilities(HANDLE hDevice) {
	PHIDP_PREPARSED_DATA pPreparsedData = GetPreparsedData(hDevice);
	return GetDeviceCapabilities(hDevice, NULL);
}

HIDP_CAPS TouchpadReader::GetDeviceCapabilities(HANDLE hDevice, PHIDP_PREPARSED_DATA pPreparsedData) {
    if (pPreparsedData) {
        HIDP_CAPS caps;
        if (HidP_GetCaps(pPreparsedData, &caps) == HIDP_STATUS_SUCCESS)
            return caps;
    }
}

PHIDP_VALUE_CAPS TouchpadReader::GetDeviceCapabilityValues(HIDP_CAPS caps, PHIDP_PREPARSED_DATA pPreparsedData) {
    USHORT valueCapsLength = caps.NumberInputValueCaps;

    PHIDP_VALUE_CAPS pValueCaps = (PHIDP_VALUE_CAPS)malloc(caps.NumberInputValueCaps * sizeof(HIDP_VALUE_CAPS));
    if (pValueCaps && HidP_GetValueCaps(HidP_Input, pValueCaps, &valueCapsLength, pPreparsedData) == HIDP_STATUS_SUCCESS)
        return pValueCaps;
    
    free(pValueCaps);
    return false;
}


BOOL TouchpadReader::IsTouchpadDevice(HANDLE hDevice) {
    return IsTouchpadDevice(hDevice, GetPreparsedData(hDevice));
}

BOOL TouchpadReader::IsTouchpadDevice(HANDLE hDevice, PHIDP_PREPARSED_DATA pPreparsedData) {
    bool isTouchpad = false;
    if (pPreparsedData)
    {
        HIDP_CAPS caps;
        if (HidP_GetCaps(pPreparsedData, &caps) == HIDP_STATUS_SUCCESS)
            isTouchpad = (caps.UsagePage == 0x0D && caps.Usage == 0x05);
    }
    return isTouchpad;
}

bool TouchpadReader::GetTouchpadDimensions(HANDLE hDevice, DIMENSIONS& size)
{
	PHIDP_PREPARSED_DATA pPreparsed = GetPreparsedData(hDevice);
	HIDP_CAPS caps = GetDeviceCapabilities(hDevice, pPreparsed);
    PHIDP_VALUE_CAPS pValueCaps = GetDeviceCapabilityValues(caps, pPreparsed);

    LONG xMin = 0, xMax = 0;
    LONG yMin = 0, yMax = 0;

    for (USHORT i = 0; i < caps.NumberInputValueCaps; i++)
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

TOUCHPAD_DATA TouchpadReader::ProcessInput(HRAWINPUT hRawInput)
{

    UINT size = 0;

    GetRawInputData(hRawInput, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

    BYTE* buffer = new BYTE[size];
    if (GetRawInputData(hRawInput, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size)
    {
        RAWINPUT* raw = (RAWINPUT*)buffer;

        if (raw->header.dwType == RIM_TYPEHID)
        {
            if (IsTouchpadDevice(raw->header.hDevice))
            {
                RAW_TOUCHPAD_EVENT* touch = (RAW_TOUCHPAD_EVENT*)(raw->data.hid.bRawData);

                touchpadData.touchCount = touch->fingers >> 4;

                for (size_t i = 0; i < touchpadData.touchCount; i++)
                {
                    RAW_TOUCH_POSITION position = touch->positions[i];
                    UINT x = (((WORD)position.x.high) << 8) + position.x.low;
                    UINT y = (((WORD)position.y.high) << 8) + position.y.low;


                    RAW_TOUCH_SIZE size = touch->sizes[i];
                    int width = size.dimensions >> 4;
                    int height = size.dimensions & 0b00001111;

                    touchpadData.touches[i].position.x = x;
                    touchpadData.touches[i].position.y = y;
                    touchpadData.touches[i].dimensions.width = width;
                    touchpadData.touches[i].dimensions.height = height;
                    touchpadData.touches[i].size = size.size;
                }

                return touchpadData;
            }
            else OutputDebugString(L"Ignoring non-touchpad HID input\n");
        }
    }
    delete[] buffer;

    return {};
}