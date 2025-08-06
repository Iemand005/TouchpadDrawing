#include "TouchpadReader.h"

TouchpadReader::TouchpadReader(HWND hWnd){
	RegisterWindow(hWnd);
    RegisterRawInputDevice(hWnd);
}

TouchpadReader::~TouchpadReader()
{
}

bool TouchpadReader::RegisterWindow(HWND hWnd) {
    return RegisterTouchWindow(hWnd, TWF_FINETOUCH | TWF_WANTPALM);
}

bool TouchpadReader::RegisterRawInputDevice(HWND hWnd) {
    UINT deviceCount = 0;

    SetProp(hWnd, L"MicrosoftTabletPenServiceProperty", (HANDLE)0x00000001);

    RAWINPUTDEVICE rid;

    rid.usUsagePage = 0x0D;  // Digitizer
    rid.usUsage = 0x05;      // Touchpad
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hWnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        // Error handling
        OutputDebugString(L"Failed to register for raw input\n");
    }

    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hWnd;

    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
    {
        // Error handling
        OutputDebugString(L"Failed to register for raw input\n");
    }

    return true;
}

PHIDP_PREPARSED_DATA TouchpadReader::GetPreparsedData(HANDLE hDevice) {
    UINT preparsedSize = 0;
    if (GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, NULL, &preparsedSize) == 0) {
        PHIDP_PREPARSED_DATA pPreparsedData = (PHIDP_PREPARSED_DATA)malloc(preparsedSize);

        if (pPreparsedData && GetRawInputDeviceInfo(hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &preparsedSize) != 0)
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
    
    return {};
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

DIMENSIONS TouchpadReader::GetTouchpadDimensions(HANDLE hDevice)
{
	PHIDP_PREPARSED_DATA pPreparsed = GetPreparsedData(hDevice);
	HIDP_CAPS caps = GetDeviceCapabilities(hDevice, pPreparsed);
    PHIDP_VALUE_CAPS pValueCaps = GetDeviceCapabilityValues(caps, pPreparsed);

    LONG xMin = 0, xMax = 0;
    LONG yMin = 0, yMax = 0;

    for (USHORT i = 0; i < caps.NumberInputValueCaps; i++)
    {
        if (pValueCaps[i].UsagePage == 0x01)
        {
            if (pValueCaps[i].Range.UsageMin == 0x30)
            {
                xMin = pValueCaps[i].LogicalMin;
                xMax = pValueCaps[i].LogicalMax;
            }
            else if (pValueCaps[i].Range.UsageMin == 0x31)
            {
                yMin = pValueCaps[i].LogicalMin;
                yMax = pValueCaps[i].LogicalMax;
            }
        }
    }

    DIMENSIONS dimensions;
    dimensions.width = xMax - xMin;
    dimensions.height = yMax - yMin;

    free(pValueCaps);
    free(pPreparsed);
    return dimensions;
}

TOUCHPAD_EVENT TouchpadReader::ProcessInput(HRAWINPUT hRawInput)
{

    UINT size = 0;

    GetRawInputData(hRawInput, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));

    BYTE* buffer = new BYTE[size];
    if (GetRawInputData(hRawInput, RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER)) == size)
    {
        RAWINPUT* raw = (RAWINPUT*)buffer;

        if (raw->header.dwType == RIM_TYPEMOUSE) {
            
			BOOL pressed = raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN;

			touchpadData.buttonPressed = pressed;
        }
        
        if (raw->header.dwType == RIM_TYPEHID)
        {
            touchpadData.touchpadSize = GetTouchpadDimensions(raw->header.hDevice);
            if (IsTouchpadDevice(raw->header.hDevice))
            {
                RAW_TOUCHPAD_EVENT* touch = (RAW_TOUCHPAD_EVENT*)(raw->data.hid.bRawData);

                touchpadData.touchCount = touch->fingers >> 4;

                for (size_t i = 0; i < touchpadData.touchCount; i++)
                {
                    RAW_TOUCH_POSITION position = touch->positions[i];

                    UINT x = (((WORD)position.x.high) << 8) + position.x.low;
                    UINT y = (((WORD)position.y.high) << 8) + position.y.low;
					UINT id = position.idAndEventType >> 4;
					UINT eventType = position.idAndEventType & 0b00001111;

                    RAW_TOUCH_SIZE size = touch->sizes[i];
                    int width = size.dimensions >> 4;
                    int height = size.dimensions & 0b00001111;

					touchpadData.touches[i].eventType = eventType;
                    touchpadData.touches[i].touch.id = id;
                    touchpadData.touches[i].touch.position.x = x;
                    touchpadData.touches[i].touch.position.y = y;
                    touchpadData.touches[i].touch.dimensions.width = width;
                    touchpadData.touches[i].touch.dimensions.height = height;
                    touchpadData.touches[i].touch.size = size.size;
                }

                return touchpadData;
            }
            else OutputDebugString(L"Ignoring non-touchpad HID input\n");
        }
    }
    delete[] buffer;

    return {};
}