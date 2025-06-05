#pragma once
#include "Delegate.h"
#include <windows.h>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include <string>
#include "InputDefines.h"
#include "InputContext.h"

#include "Debug.h"

class UInputManager
{
private:
    struct FContextKey
    {
        int Priority;
        std::string Name;

        // 우선순위 기준 정렬 (낮은 값이 높은 우선순위)
        bool operator<(const FContextKey& Other) const
        {
            if (Priority != Other.Priority)
                return Priority < Other.Priority;
            return Name < Other.Name; // 동일 우선순위 시 이름순
        }
    };

private:
    std::unordered_map<WPARAM, bool> KeyStates;

    // 컨텍스트 우선순위 정렬 맵
    std::map<FContextKey, std::shared_ptr<UInputContext>> SortedContexts;
    // 빠른 이름 기반 조회를 위한 보조 인덱스
    std::unordered_map<std::string, FContextKey> NameToKeyMap;

    UInputManager()
    {
        // 시스템 컨텍스트 초기화
        SystemContext = std::make_shared<UInputContext>(SYSTEM_CONTEXT_NAME, SYSTEM_CONTEXT_PRIORITY);
        RegisterInputContext(SystemContext);
    }

    // 복사 및 이동 방지
    UInputManager(const UInputManager&) = delete;
    UInputManager& operator=(const UInputManager&) = delete;
    UInputManager(UInputManager&&) = delete;
    UInputManager& operator=(UInputManager&&) = delete;

public:

    //최하위 우선순위 시스템 컨텍스트
    static constexpr int SYSTEM_CONTEXT_PRIORITY = INT32_MAX; // 최하위 우선순위
    static constexpr const char* SYSTEM_CONTEXT_NAME = "SystemContext";
    std::shared_ptr<UInputContext> SystemContext;

public:
    static UInputManager* Get()
    {
        static UInputManager* Instance = []() {
            UInputManager* manager = new UInputManager();
            return manager;
            }();
        return Instance;
    }

    // 입력 컨텍스트 등록
    void RegisterInputContext(const std::shared_ptr<UInputContext>& Context)
    {
        if (!Context) return;

        const std::string& ContextName = Context->GetName();

        // 이미 있으면 제거 후 다시 추가 (우선순위 변경 가능성)
        auto nameIt = NameToKeyMap.find(ContextName);
        if (nameIt != NameToKeyMap.end())
        {
            SortedContexts.erase(nameIt->second);
            NameToKeyMap.erase(nameIt);
        }

        // 새 키로 등록
        FContextKey NewKey{ Context->GetPriority(), ContextName };
        SortedContexts[NewKey] = Context;
        NameToKeyMap[ContextName] = NewKey;
    }

    // 입력 컨텍스트 제거
    void UnregisterInputContext(const std::string& ContextName)
    {
        // 시스템 컨텍스트는 제거 불가
        if (ContextName == SYSTEM_CONTEXT_NAME)
            return;

        // 1. 기존 컨텍스트 찾기
        auto nameIt = NameToKeyMap.find(ContextName);
        if (nameIt == NameToKeyMap.end())
            return; // 존재하지 않는 컨텍스트

        const FContextKey& OldKey = nameIt->second;

        // 2. 기존 컨텍스트 추출
        auto contextIt = SortedContexts.find(OldKey);
        if (contextIt == SortedContexts.end())
        {
            // 데이터 무결성 오류 - 보조 인덱스와 메인 맵 불일치
            LOG_FUNC_CALL("[ERROR] Data integrity - mismatch between auxiliary index and main map");
            NameToKeyMap.erase(nameIt);
            return;
        }

        // 3. 기존 항목 제거
        SortedContexts.erase(contextIt);
        NameToKeyMap.erase(nameIt);
    }

    // 컨텍스트 조회 (O(1) 복잡도)
    std::shared_ptr<UInputContext> GetContext(const std::string& ContextName)
    {
        auto nameIt = NameToKeyMap.find(ContextName);
        if (nameIt != NameToKeyMap.end())
        {
            auto contextIt = SortedContexts.find(nameIt->second);
            return (contextIt != SortedContexts.end()) ? contextIt->second : nullptr;
        }
        return nullptr;
    }

    void UpdateContextPriority(const std::string& ContextName, int NewPriority)
    {
        // 시스템 컨텍스트는 우선순위 변경 불가
        if (ContextName == SYSTEM_CONTEXT_NAME)
            return;

        // 1. 기존 컨텍스트 찾기
        auto nameIt = NameToKeyMap.find(ContextName);
        if (nameIt == NameToKeyMap.end())
            return; // 존재하지 않는 컨텍스트

        const FContextKey& OldKey = nameIt->second;

        // 2. 우선순위가 동일하면 변경 불필요
        if (OldKey.Priority == NewPriority)
            return;

        // 3. 기존 컨텍스트 추출
        auto contextIt = SortedContexts.find(OldKey);
        if (contextIt == SortedContexts.end())
        {
            // 데이터 무결성 오류 - 보조 인덱스와 메인 맵 불일치
            LOG_FUNC_CALL("[ERROR] Data integrity - mismatch between auxiliary index and main map");
            NameToKeyMap.erase(nameIt);
            return;
        }

        std::shared_ptr<UInputContext> Context = contextIt->second;

        // 4. 기존 항목 제거
        SortedContexts.erase(contextIt);
        NameToKeyMap.erase(nameIt);

        // 5. 컨텍스트 내부 우선순위 업데이트
        Context->SetPriority(NewPriority);

        // 6. 새 키로 재등록
        FContextKey NewKey{ NewPriority, ContextName };
        SortedContexts[NewKey] = Context;
        NameToKeyMap[ContextName] = NewKey;
    }

    // Windows 메시지 처리 (확장)
    bool ProcessWindowsMessage(UINT Message, WPARAM WParam, LPARAM LParam)
    {
        switch (Message)
        {
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            {
                const bool bWasPressed = KeyStates[WParam];
                KeyStates[WParam] = true;
                FKeyEventData EventData = CreateEventData(Message, WParam, LParam, EKeyEvent::Pressed);

                bool bIsRepeat = (HIWORD(LParam) & KF_REPEAT) == KF_REPEAT;

                if (!bWasPressed)
                {
                    return ProcessKeyEventData(EventData);
                }
                if (bIsRepeat)
                {
                    EventData.EventType = EKeyEvent::Repeat;
                    return ProcessKeyEventData(EventData);
                }

                return true;
            }

            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                KeyStates[WParam] = false;
                FKeyEventData EventData = CreateEventData(Message, WParam, LParam, EKeyEvent::Released);
                return ProcessKeyEventData(EventData);
            }
        }

        return false;
    }


    // 키 상태 조회
    bool IsKeyDown(WPARAM KeyCode) const
    {
        auto It = KeyStates.find(KeyCode);
        return It != KeyStates.end() && It->second;
    }

    // 모디파이어와 함께 키 상태 조회
    bool IsKeyDownWithModifiers(WPARAM KeyCode, bool bCheckAlt, bool bCheckCtrl, bool bCheckShift) const
    {
        if (!IsKeyDown(KeyCode)) return false;

        if (bCheckAlt && !(GetAsyncKeyState(VK_MENU) & 0x8000)) return false;
        if (bCheckCtrl && !(GetAsyncKeyState(VK_CONTROL) & 0x8000)) return false;
        if (bCheckShift && !(GetAsyncKeyState(VK_SHIFT) & 0x8000)) return false;

        return true;
    }

#pragma region DEBUG
#ifdef _DEBUG
    void ValidateDataIntegrity() const
    {
        // NameToKeyMap과 SortedContexts 동기화 확인
        for (const auto& [Name, Key] : NameToKeyMap)
        {
            auto it = SortedContexts.find(Key);
            assert(it != SortedContexts.end() && "Data integrity violation detected");
            assert(it->second->GetName() == Name && "Name mismatch detected");
        }
    }
#endif
#pragma endregion
private:
    FKeyEventData CreateEventData(UINT Message, WPARAM WParam, LPARAM LParam, EKeyEvent InEventType)
    {
        FKeyEventData EventData;
        EventData.KeyCode = WParam;
        EventData.EventType = InEventType;
        EventData.bAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        EventData.bControl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        EventData.bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        return EventData;
    }

    bool ProcessKeyEventData(const FKeyEventData& InEventData)
    {
        // 정렬된 컨텍스트 순회
        for (const auto& [Key, Context] : SortedContexts)
        {
            if (Context->IsActive() && Context->ProcessInput(InEventData))
                return true;
        }
        return false;
    }


};