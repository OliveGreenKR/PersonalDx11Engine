#pragma once
#include "Delegate.h"
#include "StringHash.h"
#include <windows.h>
#include <vector>
#include <cstring>

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
using FOnKeyEvent = TDelegate<const FKeyEventData&>;

// 입력 액션 클래스
class UInputAction
{
public:
    std::vector<WPARAM> KeyCodes;

private:
    static constexpr size_t MAX_ACTION_NAME = 64;

    FStringHash ActionHash;
    char ActionName[MAX_ACTION_NAME];

public:
    // 기본 생성자
    UInputAction() : ActionHash()
    {
        ActionName[0] = '\0';
    }

    // 문자열 생성자
    explicit UInputAction(const char* InName)
        : ActionHash(InName)
    {
        if (InName)
        {
            strncpy_s(ActionName, InName, MAX_ACTION_NAME - 1);
            ActionName[MAX_ACTION_NAME - 1] = '\0';
        }
        else
        {
            ActionName[0] = '\0';
        }
    }

    // 복사 생성자
    UInputAction(const UInputAction& InAction)
        : ActionHash(InAction.ActionHash), KeyCodes(InAction.KeyCodes)
    {
        strncpy_s(ActionName, InAction.ActionName, MAX_ACTION_NAME - 1);
        ActionName[MAX_ACTION_NAME - 1] = '\0';
    }

    // 대입 연산자
    UInputAction& operator=(const UInputAction& InAction)
    {
        if (this != &InAction)
        {
            ActionHash = InAction.ActionHash;
            KeyCodes = InAction.KeyCodes;
            strncpy_s(ActionName, InAction.ActionName, MAX_ACTION_NAME - 1);
            ActionName[MAX_ACTION_NAME - 1] = '\0';
        }
        return *this;
    }

    // 접근자
    const FStringHash& GetHash() const { return ActionHash; }
    const char* GetName() const { return ActionName; }

    // 비교 연산자
    bool operator==(const UInputAction& Other) const
    {
        return ActionHash == Other.ActionHash;
    }

    bool operator!=(const UInputAction& Other) const
    {
        return ActionHash != Other.ActionHash;
    }

    bool operator<(const UInputAction& Other) const
    {
        return ActionHash < Other.ActionHash;
    }

    // 유효성 검사
    bool IsValid() const { return ActionHash.IsValid(); }
    bool MatchesHash(const FStringHash& Hash) const
    {
        return ActionHash == Hash;
    }
};