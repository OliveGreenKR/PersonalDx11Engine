#include "InputManager.h"
#include <windows.h>
#include "Math.h"

#define KEY_UP 'W'
#define KEY_DOWN 'S'
#define KEY_LEFT 'A'
#define KEY_RIGHT 'D'

#define KEY_UP2 'I'
#define KEY_DOWN2 'K'
#define KEY_LEFT2 'J'
#define KEY_RIGHT2 'L'


bool UInputManager::ProcessWindowsMessage(UINT Message, WPARAM WParam, LPARAM LParam)
{
    switch (Message)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            const bool bWasPressed = KeyStates[WParam];
            KeyStates[WParam] = true;

            FKeyEventData EventData;
            EventData.KeyCode = WParam;
            EventData.bAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            EventData.bControl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            EventData.bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

            // 반복 입력 확인
            bool bIsRepeat = (HIWORD(LParam) & KF_REPEAT) == KF_REPEAT;

            if (!bWasPressed)
            {
                EventData.EventType = EKeyEvent::Pressed;
                KeyEventDelegates[EKeyEvent::Pressed].Broadcast(EventData);
            }
            else if (bIsRepeat)
            {
                EventData.EventType = EKeyEvent::Repeat;
                KeyEventDelegates[EKeyEvent::Repeat].Broadcast(EventData);
            }

            return true;
        }

        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            KeyStates[WParam] = false;

            FKeyEventData EventData;
            EventData.KeyCode = WParam;
            EventData.EventType = EKeyEvent::Released;
            EventData.bAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
            EventData.bControl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
            EventData.bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

            KeyEventDelegates[EKeyEvent::Released].Broadcast(EventData);
            return true;
        }
    }


    return false;
}