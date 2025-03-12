#pragma once
#include "Delegate.h"
#include <windows.h>
#include <unordered_map>
#include <memory>
#include <string>


// 키 이벤트 타입 정의
enum class EKeyEvent
{
    Pressed,
    Released,
    Repeat,
};

// 입력 이벤트에 전달될 데이터 구조체
struct FKeyEventData
{
    WPARAM KeyCode;
    EKeyEvent EventType;
    bool bAlt;
    bool bControl;
    bool bShift;
};

// 키 이벤트 델리게이트 타입 정의
using FOnKeyEvent = FDelegate<const FKeyEventData&>;

class UInputManager
{
private:
    // 각 이벤트 타입별 델리게이트
    std::unordered_map<EKeyEvent, FOnKeyEvent> KeyEventDelegates;

    // 현재 키 상태 저장
    std::unordered_map<WPARAM, bool> KeyStates;

    UInputManager() = default;

public:
    static UInputManager* Get()
    {
        static std::unique_ptr<UInputManager> Instance = std::unique_ptr<UInputManager>(new UInputManager());
        return Instance.get();
    }

    // 윈도우 프로시저에서 호출될 메시지 처리 함수
    bool ProcessWindowsMessage(UINT Message, WPARAM WParam, LPARAM LParam);

    //시스템 바운딩
    void BindKeyEventSystem(
        EKeyEvent EventType,
        const std::function<void(const FKeyEventData&)>& InFunction,
        const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].BindSystem(InFunction, InFunctionName);
    }
    //시스템 언바운딩
    void UnbindKeyEventSystem(
        EKeyEvent EventType,
        const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].UnbindSystem(InFunctionName);
    }

    // 특정 키 이벤트에 대한 함수 바인딩
    template<typename T>
    void BindKeyEvent(
        EKeyEvent EventType,
        const std::shared_ptr<T>& InObject,
        const std::function<void(const FKeyEventData&)>& InFunction,
        const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].Bind(InObject, InFunction, InFunctionName);
    }

    // 특정 키 이벤트에서 함수 언바인딩
    template<typename T>
    void UnbindKeyEvent(
        EKeyEvent EventType,
        const std::shared_ptr<T>& InObject,
        const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].Unbind(InObject, InFunctionName);
    }

    // 객체의 모든 키 이벤트 바인딩 제거
    template<typename T>
    void UnbindAllKeyEvents(const std::shared_ptr<T>& InObject)
    {
        for (auto& Delegate : KeyEventDelegates)
        {
            Delegate.second.UnbindAll(InObject);
        }
    }

    // 특정 키가 현재 눌려있는지 확인
    bool IsKeyDown(WPARAM KeyCode) const
    {
        auto It = KeyStates.find(KeyCode);
        return It != KeyStates.end() && It->second;
    }

    // 만약 Alt, Ctrl, Shift와 함께 눌렸는지 확인이 필요한 경우
    bool IsKeyDownWithModifiers(WPARAM KeyCode, bool bCheckAlt, bool bCheckCtrl, bool bCheckShift) const
    {
        if (!IsKeyDown(KeyCode)) return false;

        if (bCheckAlt && !(GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
        if (bCheckCtrl && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
        if (bCheckShift && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) return false;

        return true;
    }
};
