#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT NTDDI_WIN10_RS5

#include <windows.h>
#include <winuser.h>

#include <memory>
#include <string>
#include <vector>
#include <chrono>

#include "Touch.h"

using namespace std;
using namespace chrono;

class TouchEmulator
{
public:

    //void UpdateScreenDimensions();

    TouchEmulator() {
        InitializeTouchInjection(1, TOUCH_FEEDBACK_DEFAULT);
        UpdateScreenDimensions();

        hDevice = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_INDIRECT);
    }



    void SendTouchInput(int x, int y, int id, bool isDown) {
        static auto lastCall = steady_clock::now();
        auto now = steady_clock::now();

        auto duration = duration_cast<milliseconds>(now - lastCall).count();

        if (duration < minDelayMs) {
            //return; // Prevent sending too many inputs too quickly
		}

        POINTER_TOUCH_INFO contact;
        memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));

        contact.pointerInfo.pointerType = PT_TOUCH;
        contact.pointerInfo.pointerId = id;
        contact.pointerInfo.ptPixelLocation.x = x;
        contact.pointerInfo.ptPixelLocation.y = y;

        if (isDown) {
            contact.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN;
        }
        else {
            contact.pointerInfo.pointerFlags = POINTER_FLAG_UP;
        }

        contact.touchFlags = TOUCH_FLAG_NONE;
        contact.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION;
        contact.orientation = 90;
        contact.rcContact.left = x - 2;
        contact.rcContact.top = y - 2;
        contact.rcContact.right = x + 2;
        contact.rcContact.bottom = y + 2;

        InjectTouchInput(1, &contact);
    }

    void GetScreenResolution(UINT& width, UINT& height) {
        /*    RECT desktopRect;
            GetClientRect(GetDesktopWindow(), &desktopRect);
            width = desktopRect.right - desktopRect.left;
            height = desktopRect.bottom - desktopRect.top;
        */

        width = GetSystemMetrics(SM_CXSCREEN);
        height = GetSystemMetrics(SM_CYSCREEN);
    }


    void UpdateScreenDimensions() {
        GetScreenResolution(screenDimensions.width, screenDimensions.height);
    }

    POINT TransformTouchToDisplayPosition(POINT touchPosition, DIMENSIONS touchAreaSize) {
        FLOAT normalizedX = touchPosition.x / (FLOAT)touchAreaSize.width;
        FLOAT normalizedY = touchPosition.y / (FLOAT)touchAreaSize.height;

        LONG screenX = normalizedX * screenDimensions.width;
        LONG screenY = normalizedY * screenDimensions.height;
		
        return { screenX, screenY };
    }

    steady_clock::time_point lastCall = steady_clock::now();
    
    BOOL SendTouchInputs(TOUCHPAD_EVENT touchpadEvent) {

        auto now = steady_clock::now();

        auto duration = duration_cast<milliseconds>(now - lastCall).count();

        INT touchCount = touchpadEvent.touchCount;
        TOUCH_EVENT* touches = touchpadEvent.touches;

        if (touchCount > maxTouches)
            touchCount = maxTouches;

        POINTER_TOUCH_INFO contacts[maxTouches] = { 0 };
        if (!contacts) return false;

        BOOL currentActiveTouches[maxTouches];


        for (UINT i = 0; i < touchCount; ++i) {

            TOUCH_EVENT event = touches[i];
            TOUCH touch = event.touch;


            POINT displayPosition = TransformTouchToDisplayPosition(touch.position, touchpadEvent.touchpadSize);

            POINTER_FLAGS pointerFlags;

            if (event.eventType == 3) {

                pointerFlags = POINTER_FLAG_INRANGE;

                down = touch.size > touchSizeThreshold;
                
                if (down)
                    pointerFlags |= POINTER_FLAG_INCONTACT;

                if (!activeTouches[touch.id]) {
                    OutputDebugString(L"New touch detected\n");

                    isPenActive = touch.size < penDownThreshold;
                    activeTouches[touch.id] = true;
                }
                else {
                    pointerFlags |= POINTER_FLAG_UPDATE;
                }

            }
            else {
                pointerFlags = POINTER_FLAG_UP;
                activeTouches[touch.id] = FALSE;
                OutputDebugString(L"Touch went up\n");
            }

            if (touch.id == 0 && isPenActive) {

                if (pointerFlags & POINTER_FLAG_UPDATE) {
                    if (duration < minDelayMs) {
                        OutputDebugString(L"Touch input throttled\n");

                        return false;
                    }
                }

                int newMouseDown = (GetAsyncKeyState(VK_LBUTTON));

                OutputDebugString(to_wstring(newMouseDown).c_str());

                SendPenInput(touches[0].touch, touchpadEvent.touchpadSize, pointerFlags);
                lastCall = now;

                return true;
            }


            FLOAT touchAspectRatio = (FLOAT)touch.dimensions.width / (FLOAT)touch.dimensions.height;
            UINT touchWidth = (UINT)(touch.size * touchAspectRatio);
            UINT touchHeight = (UINT)(touch.size / touchAspectRatio);

            contacts[i].pointerInfo.pointerType = PT_TOUCH;
            contacts[i].pointerInfo.pointerId = touch.id;
            contacts[i].pointerInfo.ptPixelLocation = displayPosition;
            contacts[i].pointerInfo.pointerFlags = pointerFlags;

            contacts[i].touchFlags = TOUCH_FLAG_NONE;
            contacts[i].touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION;
            contacts[i].orientation = 90;
            contacts[i].rcContact.left = touch.position.x - 2;
            contacts[i].rcContact.top = touch.position.y - 2;
            contacts[i].rcContact.right = touch.position.x + 2;
            contacts[i].rcContact.bottom = touch.position.y + 2;

        }

        if (!simulateTouch) return true;

        BOOL success = InjectTouchInput(touchCount, contacts);

        return success;
    }

    BOOL SendPenInput(TOUCH touch, DIMENSIONS touchAreaSize, POINTER_FLAGS pointerFlags) {

        static auto lastCall = steady_clock::now();
        auto now = steady_clock::now();

        auto duration = duration_cast<milliseconds>(now - lastCall).count();

        if (duration < minDelayMs) {
            return false;
        }

        //HSYNTHETICPOINTERDEVICE device = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_INDIRECT);
		if (!hDevice) hDevice = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_INDIRECT);
        if (!hDevice) return false;

        POINT position = TransformTouchToDisplayPosition(touch.position, touchAreaSize);

        POINTER_TYPE_INFO inputInfo = {};
        inputInfo.type = PT_PEN;
        inputInfo.penInfo.pointerInfo.pointerType = PT_PEN;
        inputInfo.penInfo.pointerInfo.pointerId = 0;
        inputInfo.penInfo.pointerInfo.frameId = ++penFrameId;
        inputInfo.penInfo.pointerInfo.pointerFlags = pointerFlags;
        inputInfo.penInfo.penMask = PEN_MASK_NONE;
        inputInfo.penInfo.pointerInfo.ptPixelLocation = position;


        BOOL injected = InjectSyntheticPointerInput(hDevice, &inputInfo, 1);

        if (!injected) {
			OutputDebugString(L"Failed to inject pen input\n");
        }

        return injected;
    }

private:
    static const int maxTouches = 5;

    BOOL activeTouches[maxTouches];
    BOOL down = FALSE;

	DIMENSIONS screenDimensions;

    HSYNTHETICPOINTERDEVICE hDevice;

    bool g_penActive = false;



    int minDelayMs = 240 / 1000;

	int penFrameId = 0;

	bool isPenActive = false;
	UINT touchSizeThreshold = 15;
    UINT penDownThreshold = 18;

    BOOL simulateTouch = false;
    BOOL simulateHover = true;
};

