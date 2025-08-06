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
    
    void SendTouchInputs(TOUCHPAD_EVENT touchpadEvent) {

        auto now = steady_clock::now();

        auto duration = duration_cast<milliseconds>(now - lastCall).count();

        /*OutputDebugString(to_wstring(duration).c_str());
        if (duration < minDelayMs) {
            OutputDebugString(L"Touch input throttled\n");
            
            return;
        }*/



        INT touchCount = touchpadEvent.touchCount;
        TOUCH_EVENT* touches = touchpadEvent.touches;

        if (touchCount > maxTouches)
            touchCount = maxTouches;

        POINTER_TOUCH_INFO contacts[maxTouches] = { 0 };
        if (!contacts) return;

        BOOL currentActiveTouches[maxTouches];

		

        /*for (UINT i = 0; i < 5; ++i)
            currentActiveTouches[i] = FALSE;*/

        for (UINT i = 0; i < touchCount; ++i) {

            TOUCH_EVENT event = touches[i];
            TOUCH touch = event.touch;

            /*FLOAT normalizedX = touch.position.x / (FLOAT)touchpadEvent.touchpadSize.width;
            FLOAT normalizedY = touch.position.y / (FLOAT)touchpadEvent.touchpadSize.height;

            FLOAT screenX = normalizedX * screenDimensions.width;
            FLOAT screenY = normalizedY * screenDimensions.height;*/

            POINT displayPosition = TransformTouchToDisplayPosition(touch.position, touchpadEvent.touchpadSize);

            POINTER_FLAGS pointerFlags;

            if (event.eventType == 3) {
                OutputDebugString(L"Touch update event\n");

                pointerFlags;

                if (activeTouches[touch.id]) {
                    OutputDebugString(L"Touch already active, updating\n");
                    pointerFlags |= POINTER_FLAG_UPDATE;
                }
                else {
                    OutputDebugString(L"New touch detected\n");
                    pointerFlags |= POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
                }

                activeTouches[touch.id] = true;
            }
            else {
                pointerFlags = POINTER_FLAG_UP;
                activeTouches[touch.id] = FALSE;
                OutputDebugString(L"Touch went up\n");
            }

            if (touch.id == 0) {

                if (pointerFlags & POINTER_FLAG_UPDATE) {
                    OutputDebugString(to_wstring(duration).c_str());
                    if (duration < minDelayMs) {
                        OutputDebugString(L"Touch input throttled\n");

                        return;
                    }
                }

                SendPenInput(touches[0].touch, touchpadEvent.touchpadSize, pointerFlags);
                lastCall = now;

                return;
            }

            OutputDebugString(std::to_wstring(touch.id).c_str());
            OutputDebugString(TEXT(" "));

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
            /*contacts[i].rcContact.left = touch.position.x - touchWidth / 2;
            contacts[i].rcContact.top = touch.position.y - touchHeight / 2;
            contacts[i].rcContact.right = touch.position.x + touchWidth / 2;
            contacts[i].rcContact.bottom = touch.position.y + touchHeight / 2;*/


        }

        InjectTouchInput(touchCount, contacts);
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

        POINTER_TYPE_INFO inputInfo[1] = {};
        inputInfo[0].type = PT_PEN;
        inputInfo[0].penInfo.pointerInfo.pointerType = PT_PEN;
        inputInfo[0].penInfo.pointerInfo.pointerId = 0;
        inputInfo[0].penInfo.pointerInfo.frameId = penFrameId++;
        inputInfo[0].penInfo.pointerInfo.pointerFlags = pointerFlags;
        inputInfo[0].penInfo.penMask = PEN_MASK_NONE;
        inputInfo[0].penInfo.pointerInfo.ptPixelLocation = position;


        BOOL injected = InjectSyntheticPointerInput(hDevice, inputInfo, 1);
        return injected;
    }

private:
    static const int maxTouches = 5;

    BOOL activeTouches[maxTouches];

	DIMENSIONS screenDimensions;

    HSYNTHETICPOINTERDEVICE hDevice;

    bool g_penActive = false;

    int minDelayMs = 1000;

	int penFrameId = 0;
};

