#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0602
#include <windows.h>
#include <winuser.h>

#include <memory>
#include <string>

#include "Touch.h"

class TouchEmulator
{
public:

    TouchEmulator() {
        InitializeTouchInjection(1, TOUCH_FEEDBACK_DEFAULT);
    }

    void SendTouchInput(int x, int y, int id, bool isDown) {
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

    void SendTouchInputs(TOUCH* touches, const UINT touchCount) {
        POINTER_TOUCH_INFO contacts[5] = { 0 };
        if (!contacts) return;

        BOOL currentActiveTouches[5];

        for (UINT i = 0; i < 5; ++i)
            currentActiveTouches[i] = FALSE;


        for (UINT i = 0; i < touchCount; ++i) {

            
			UINT touchIndex = touches[i].id;
			currentActiveTouches[touchIndex] = TRUE;
			BOOL touchWentDown = !activeTouches[touchIndex];

            TOUCH& touch = touches[i];
            /*SendTouchInput(touch.position.x, touch.position.y, touch.id - 1, true);
            SendTouchInput(touch.position.x, touch.position.y, touch.id - 1, false);*/

            OutputDebugString(std::to_wstring(touch.id).c_str());

			FLOAT touchAspectRatio = (FLOAT)touch.dimensions.width / (FLOAT)touch.dimensions.height;
			UINT touchWidth = (UINT)(touch.size * touchAspectRatio);
			UINT touchHeight = (UINT)(touch.size / touchAspectRatio);

            contacts[i].pointerInfo.pointerType = PT_TOUCH;
            contacts[i].pointerInfo.pointerId = touch.id;
            contacts[i].pointerInfo.ptPixelLocation.x = touch.position.x;
            contacts[i].pointerInfo.ptPixelLocation.y = touch.position.y;
            contacts[i].pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;

            if (touchWentDown) {
                contacts[i].pointerInfo.pointerFlags |= POINTER_FLAG_DOWN;
                //currentActiveTouches[touchIndex] = TRUE;
				OutputDebugString(L"Touch went down\n");
            }
            else {
				contacts[i].pointerInfo.pointerFlags |= POINTER_FLAG_UPDATE;

                //OutputDebugString(L"Touch updated\n");
            }

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

        for (UINT i = 0; i < 5; ++i) {
            if (activeTouches[i] && !currentActiveTouches[i]) {
                //contacts[i].pointerInfo.pointerFlags = POINTER_FLAG_UP;
                //activeTouches[i] = FALSE;
				OutputDebugString(L"Touch went up\n");
            }

            activeTouches[i] = currentActiveTouches[i];
		}

        InjectTouchInput(touchCount, contacts);
    }

    //void SendPenInput(int x, int y, bool isDown, float pressure = 1.0f, int tiltX = 0, int tiltY = 0) {
    //    POINTER_TYPE_INFO pointerInfo = { 0 };
    //    pointerInfo.type = PT_PEN;
    //    pointerInfo.penInfo.pointerInfo.pointerType = PT_PEN;
    //    pointerInfo.penInfo.pointerInfo.pointerId = 1;  // Pen ID (0 = mouse, 1+ = pen)
    //    pointerInfo.penInfo.pointerInfo.ptPixelLocation.x = x;
    //    pointerInfo.penInfo.pointerInfo.ptPixelLocation.y = y;

    //    // Set pen flags
    //    if (isDown) {
    //        pointerInfo.penInfo.pointerInfo.pointerFlags =
    //            POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN;
    //        g_penActive = true;
    //    }
    //    else if (g_penActive) {
    //        pointerInfo.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UP;
    //        g_penActive = false;
    //    }
    //    else {
    //        pointerInfo.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE;
    //    }

    //    // Set pen-specific data (pressure, tilt)
    //    pointerInfo.penInfo.pressure = (UINT32)(pressure * 1024);  // Normalized (0-1024)
    //    pointerInfo.penInfo.tiltX = tiltX;  // -90 to +90 degrees
    //    pointerInfo.penInfo.tiltY = tiltY;

    //    INPUT input = { 0 };
    //    input.type = INPUT_HARDWARE;
    //    input.hi..pointerInfo = pointerInfo.penInfo.pointerInfo;

    //    SendInput(1, &input, sizeof(INPUT));
    //}
private:
    TOUCH activeTouches[5];

    bool g_penActive = false;
};

