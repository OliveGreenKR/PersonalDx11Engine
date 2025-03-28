#pragma once
#include "Delegate.h"
#include <windows.h>

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

//행동에 따른 키 바인딩
class UInputAction
{
public:
    UInputAction() = default;
    UInputAction(const std::string& InName) : Name(InName)
    {
    }
    // 복사 생성자
    UInputAction(const UInputAction& InAction) : Name(InAction.Name), KeyCodes(InAction.KeyCodes)
    {
    }

    const std::string& GetName() const { return Name; }
    std::vector<WPARAM> KeyCodes;

    // 대입 연산자 수정
    UInputAction& operator= (const UInputAction& InAction)
    {
        if (this != &InAction) // 자기 할당 방지
        {
            Name = InAction.Name;
            KeyCodes = InAction.KeyCodes;
        }
        return *this;
    }

private:
    std::string Name;
};