#pragma once
#include "Delegate.h"
#include <windows.h>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>
#include <cstring>
#include "InputDefines.h"
#include "InputContext.h"
#include "Debug.h"

// 입력 매니저 클래스 - 컨텍스트 관리 및 시스템 레벨 키 이벤트 처리
class UInputManager
{
private:
    // 시스템 레벨 키 이벤트 처리
    std::unordered_map<EKeyEvent, FOnKeyEvent> KeyEventDelegates;
    std::unordered_map<WPARAM, bool> KeyStates;

    // 우선순위 순으로 정렬된 컨텍스트 배열 (캐시 친화적 순회)
    std::vector<std::shared_ptr<UInputContext>> SortedContexts;

    // 시스템 컨텍스트
    static constexpr int SYSTEM_CONTEXT_PRIORITY = INT32_MAX;
    static constexpr const char* SYSTEM_CONTEXT_NAME = "SystemContext";

public:
    std::shared_ptr<UInputContext> SystemContext;

private:
    // 싱글톤 구현
    UInputManager()
    {
        // 시스템 컨텍스트 초기화
        SystemContext = std::make_shared<UInputContext>(SYSTEM_CONTEXT_NAME, SYSTEM_CONTEXT_PRIORITY);
        RegisterInputContext(SystemContext);
    }

    UInputManager(const UInputManager&) = delete;
    UInputManager& operator=(const UInputManager&) = delete;
    UInputManager(UInputManager&&) = delete;
    UInputManager& operator=(UInputManager&&) = delete;

    // 우선순위 기반 정렬
    void SortContextsByPriority()
    {
        std::sort(SortedContexts.begin(), SortedContexts.end(),
                  [](const std::shared_ptr<UInputContext>& A, const std::shared_ptr<UInputContext>& B)
                  {
                      return A->GetPriority() < B->GetPriority(); // 낮은 값이 높은 우선순위
                  });
    }

public:
    static UInputManager* Get()
    {
        static UInputManager* Instance = []() {
            UInputManager* manager = new UInputManager();
            return manager;
            }();
        return Instance;
    }

public:
    // 컨텍스트 관리
    void RegisterInputContext(const std::shared_ptr<UInputContext>& Context)
    {
        if (!Context) return;

        // 기존 컨텍스트 제거 (선형 탐색)
        SortedContexts.erase(
            std::remove_if(SortedContexts.begin(), SortedContexts.end(),
                           [&Context](const std::shared_ptr<UInputContext>& ExistingContext)
                           {
                               return ExistingContext->GetHash() == Context->GetHash();
                           }),
            SortedContexts.end());

        // 새 컨텍스트 추가 후 정렬
        SortedContexts.push_back(Context);
        SortContextsByPriority();
    }

    void UnregisterInputContext(const char* ContextName)
    {
        if (!ContextName) return;

        // 시스템 컨텍스트는 제거 불가
        if (strcmp(ContextName, SYSTEM_CONTEXT_NAME) == 0)
            return;

        FStringHash contextHash(ContextName);
        SortedContexts.erase(
            std::remove_if(SortedContexts.begin(), SortedContexts.end(),
                           [&contextHash](const std::shared_ptr<UInputContext>& Context)
                           {
                               return Context->GetHash() == contextHash;
                           }),
            SortedContexts.end());
    }

    std::shared_ptr<UInputContext> GetContext(const char* ContextName)
    {
        if (!ContextName) return nullptr;

        FStringHash contextHash(ContextName);
        for (const auto& context : SortedContexts)
        {
            if (context->GetHash() == contextHash)
            {
                return context;
            }
        }
        return nullptr;
    }

    std::shared_ptr<UInputContext> GetSystemContext()
    {
        return SystemContext;
    }

public:
    // 컨텍스트 우선순위 관리
    void SetContextPriority(const char* ContextName, int NewPriority)
    {
        if (!ContextName) return;

        // 시스템 컨텍스트는 수정 불가
        if (strcmp(ContextName, SYSTEM_CONTEXT_NAME) == 0)
            return;

        auto context = GetContext(ContextName);
        if (context)
        {
            context->SetPriority(NewPriority);
            SortContextsByPriority(); // 우선순위 변경 후 재정렬
        }
    }

public:
    // 시스템 레벨 키 이벤트 바인딩
    void BindKeyEventSystem(EKeyEvent EventType,
                            const std::function<void(const FKeyEventData&)>& InFunction,
                            const char* InFunctionName)
    {
        KeyEventDelegates[EventType].BindSystem(InFunction, InFunctionName);
    }

    void UnbindKeyEventSystem(EKeyEvent EventType, const char* InFunctionName)
    {
        auto it = KeyEventDelegates.find(EventType);
        if (it != KeyEventDelegates.end())
        {
            it->second.UnbindSystem(InFunctionName);
        }
    }

    void ClearAllKeyEventBindings()
    {
        KeyEventDelegates.clear();
    }

public:
    // Windows 메시지 처리
    bool ProcessWindowsMessage(UINT Message, WPARAM WParam, LPARAM LParam)
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

                bool bIsRepeat = (HIWORD(LParam) & KF_REPEAT) == KF_REPEAT;

                if (!bWasPressed)
                {
                    EventData.EventType = EKeyEvent::Pressed;

                    // 우선순위 순으로 컨텍스트 순회 (캐시 친화적)
                    for (const auto& Context : SortedContexts)
                    {
                        if (Context->IsActive() && Context->ProcessInput(EventData))
                        {
                            return true; // 처리 완료
                        }
                    }

                    // 컨텍스트에서 처리되지 않은 경우 시스템 레벨 처리
                    auto sysIt = KeyEventDelegates.find(EventData.EventType);
                    if (sysIt != KeyEventDelegates.end())
                    {
                        sysIt->second.Broadcast(EventData);
                    }
                }
                else if (bIsRepeat)
                {
                    EventData.EventType = EKeyEvent::Repeat;

                    for (const auto& Context : SortedContexts)
                    {
                        if (Context->IsActive() && Context->ProcessInput(EventData))
                        {
                            return true; // 처리 완료
                        }
                    }

                    // 컨텍스트에서 처리되지 않은 경우 시스템 레벨 처리
                    auto sysIt = KeyEventDelegates.find(EventData.EventType);
                    if (sysIt != KeyEventDelegates.end())
                    {
                        sysIt->second.Broadcast(EventData);
                    }
                }
                break;
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

                // 우선순위 순으로 컨텍스트 순회
                for (const auto& Context : SortedContexts)
                {
                    if (Context->IsActive() && Context->ProcessInput(EventData))
                    {
                        return true; // 처리 완료
                    }
                }

                // 컨텍스트에서 처리되지 않은 경우 시스템 레벨 처리
                auto sysIt = KeyEventDelegates.find(EventData.EventType);
                if (sysIt != KeyEventDelegates.end())
                {
                    sysIt->second.Broadcast(EventData);
                }
                break;
            }
        }
        return false;
    }

public:
    // 디버깅 및 유틸리티
    void PrintDebugInfo() const
    {
        LOG("InputManager - Registered Contexts:");
        LOG("Priority Order (Lower = Higher Priority):");

        for (const auto& context : SortedContexts)
        {
            LOG("  - %s (Priority: %d, Hash: %llu)",
                context->GetName(),
                context->GetPriority(),
                context->GetHash().GetHash());
        }

        LOG("Total Contexts: %zu", SortedContexts.size());
    }

    void PrintKeyStates() const
    {
        LOG("Current Key States:");
        for (const auto& pair : KeyStates)
        {
            if (pair.second) // 눌린 키만 출력
            {
                LOG("  - Key %c (%d): Pressed", (char)pair.first, (int)pair.first);
            }
        }
    }

    bool IsKeyPressed(WPARAM KeyCode) const
    {
        auto it = KeyStates.find(KeyCode);
        return (it != KeyStates.end()) ? it->second : false;
    }

    void ClearKeyStates()
    {
        KeyStates.clear();
        LOG("All key states cleared");
    }

    void PrintStatistics() const
    {
        LOG("InputManager Statistics:");
        LOG("  - Total Contexts: %zu", SortedContexts.size());
        LOG("  - Active Contexts: %d", CountActiveContexts());
        LOG("  - System Context Priority: %d", SYSTEM_CONTEXT_PRIORITY);
    }

private:
    int CountActiveContexts() const
    {
        int count = 0;
        for (const auto& context : SortedContexts)
        {
            if (context->IsActive())
                count++;
        }
        return count;
    }
};