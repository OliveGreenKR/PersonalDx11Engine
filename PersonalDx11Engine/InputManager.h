#pragma once
#include "Delegate.h"
#include <windows.h>
#include <unordered_map>
#include <set>
#include <memory>
#include <string>
#include "InputDefines.h"
#include "InputContext.h"

class UInputManager
{
private:
    // 기존 멤버 (키 상태 및 델리게이트)
    std::unordered_map<EKeyEvent, FOnKeyEvent> KeyEventDelegates;
    std::unordered_map<WPARAM, bool> KeyStates;

    // 이름으로 빠르게 조회하기 위한 맵
    std::unordered_map<std::string, std::shared_ptr<UInputContext>> ContextMap;

    // 우선순위 기반 정렬된 컨텍스트 목록
    struct ContextComparer {
        bool operator()(const std::shared_ptr<UInputContext>& A,
                        const std::shared_ptr<UInputContext>& B) const {
             // 낮은 값이 높은 우선순위
            return A->GetPriority() < B->GetPriority();
        }
    };
    std::set<std::shared_ptr<UInputContext>, ContextComparer> SortedContexts;

    UInputManager()
    {
        // 시스템 컨텍스트 초기화
        SystemContext = std::make_shared<UInputContext>(SYSTEM_CONTEXT_NAME, SYSTEM_CONTEXT_PRIORITY);
        RegisterInputContext(SystemContext);
    }
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
        auto existingIt = ContextMap.find(ContextName);
        if (existingIt != ContextMap.end()) {
            SortedContexts.erase(existingIt->second);
            ContextMap.erase(existingIt);
        }

        // 맵과 정렬된 셋에 모두 추가
        ContextMap[ContextName] = Context;
        SortedContexts.insert(Context);
    }

    // 입력 컨텍스트 제거
    void UnregisterInputContext(const std::string& ContextName)
    {
        // 시스템 컨텍스트는 제거 불가
        if (ContextName == SYSTEM_CONTEXT_NAME)
            return;

        auto it = ContextMap.find(ContextName);
        if (it != ContextMap.end()) {
            SortedContexts.erase(it->second);
            ContextMap.erase(it);
        }
    }

    // 컨텍스트 조회 (O(1) 복잡도)
    std::shared_ptr<UInputContext> GetContext(const std::string& ContextName)
    {
        auto it = ContextMap.find(ContextName);
        if (it != ContextMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 컨텍스트 우선순위 변경
    void SetContextPriority(const std::string& ContextName, int NewPriority)
    {
        // 시스템 컨텍스트는 수정 불가
        if (ContextName == SYSTEM_CONTEXT_NAME)
            return;

        auto context = GetContext(ContextName);
        if (context) {
            // 셋에서 제거 후 우선순위 변경하고 다시 추가
            SortedContexts.erase(context);
            context->SetPriority(NewPriority);
            SortedContexts.insert(context);
        }
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

                FKeyEventData EventData;
                EventData.KeyCode = WParam;
                EventData.bAlt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                EventData.bControl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                EventData.bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

                bool bIsRepeat = (HIWORD(LParam) & KF_REPEAT) == KF_REPEAT;

                if (!bWasPressed)
                {
                    EventData.EventType = EKeyEvent::Pressed;

                    // 정렬된 컨텍스트 순회 (우선순위 높은 순)
                    for (const auto& Context : SortedContexts)
                    {
                        if (Context->IsActive() && Context->ProcessInput(EventData))
                        {
                            return true; // 처리 완료
                        }
                    }
                }
                else if (bIsRepeat)
                {
                    EventData.EventType = EKeyEvent::Repeat;

                    // 정렬된 컨텍스트 순회
                    for (const auto& Context : SortedContexts)
                    {
                        if (Context->IsActive() && Context->ProcessInput(EventData))
                        {
                            return true;
                        }
                    }
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

                // 정렬된 컨텍스트 순회
                for (const auto& Context : SortedContexts)
                {
                    if (Context->IsActive() && Context->ProcessInput(EventData))
                    {
                        return true;
                    }
                }
                return true;
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
};