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

    RAWINPUTDEVICE rid[1];

    rid[0].usUsagePage = 0x0D;  // Digitizer
    rid[0].usUsage = 0x05;      // Touchpad
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hWnd;

    if (!RegisterRawInputDevices(rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        // Error handling
        OutputDebugString(L"Failed to register for raw input\n");
    }

    if (GetRawInputDeviceList(NULL, &deviceCount, sizeof(RAWINPUTDEVICELIST)) != -1) {

        PRAWINPUTDEVICELIST inputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * deviceCount);
        if (GetRawInputDeviceList(inputDeviceList, &deviceCount, sizeof(RAWINPUTDEVICELIST))) {

            PRAWINPUTDEVICE rawInputDevices = (PRAWINPUTDEVICE)malloc(sizeof(RAWINPUTDEVICE) * deviceCount);

            for (UINT i = 0; i < deviceCount; i++)
            {
                RAWINPUTDEVICELIST inputDevice = inputDeviceList[i];
                HANDLE hDevice = inputDevice.hDevice;
                DWORD deviceType = inputDevice.dwType;


                UINT deviceInfoSize;
                GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, NULL, &deviceInfoSize);

                RID_DEVICE_INFO deviceInfo;
                deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);

                GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &deviceInfo, &deviceInfoSize);

                if (deviceInfo.hid.usUsagePage == 0x0D && deviceInfo.hid.usUsage == 0x05) // Touchpad
                {
                    OutputDebugString(L"AKAKAK");

                    int width, height;
                    DIMENSIONS dimensions = GetTouchpadDimensions(hDevice);

                    touchpadData.touchpadSize = dimensions;
                }

                switch (inputDevice.dwType)
                {
                case RIM_TYPEMOUSE:    OutputDebugString(L"Mouse\n"); break;
                case RIM_TYPEKEYBOARD: OutputDebugString(L"Keyboard\n"); break;
                case RIM_TYPEHID:
                    OutputDebugString(L"HID\n");
                    break;
                default:               OutputDebugString(L"Unknown (%d)\n"); break;
                }

                RAWINPUTDEVICE rawInputDevice;
                rawInputDevice.usUsagePage = deviceInfo.hid.usUsagePage;
                rawInputDevice.usUsage = deviceInfo.hid.usUsage;
                rawInputDevice.dwFlags = RIDEV_INPUTSINK;
                rawInputDevice.hwndTarget = hWnd;

                if (RegisterRawInputDevices(&rawInputDevice, 1, sizeof(RAWINPUTDEVICE))) {
                    return true;
                    OutputDebugString(L"Failed to register raw input devices\n");
                }

            }
        }
        else free(inputDeviceList);
        
    }
    return false;
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

    DIMENSIONS dimensions;
    dimensions.width = xMax - xMin;
    dimensions.height = yMax - yMin;

    free(pValueCaps);
    free(pPreparsed);
    return dimensions;
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

                    //LPCWSTR hi = L"a";
                    //wsprintf(hi)
                    //OutputDebugString(std::to_wstring(touch->unk1).c_str());
                    //OutputDebugString(std::to_wstring(x).c_str());

                    RAW_TOUCH_SIZE size = touch->sizes[i];
                    int width = size.dimensions >> 4;
                    int height = size.dimensions & 0b00001111;

                    touchpadData.touches[i].id = position.index;
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