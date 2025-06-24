#pragma once
#include "InputDefines.h"
#include "StringHash.h"
#include <unordered_map>
#include <vector>
#include <cstring>
#include "Debug.h"

// 입력 컨텍스트 클래스 - 액션 기반 입력 처리 담당
class UInputContext : public std::enable_shared_from_this<UInputContext>
{
private:
    // 액션 매핑 및 델리게이트 관리
    std::unordered_map<FStringHash, UInputAction, FStringHash::Hasher> ActionMappings;
    std::unordered_map<EKeyEvent, std::unordered_map<FStringHash, FOnKeyEvent, FStringHash::Hasher>> ActionEventDelegates;
    std::unordered_map<WPARAM, std::vector<FStringHash>> KeyToActionMap;

    // 컨텍스트 정보
    static constexpr size_t MAX_CONTEXT_NAME = 32;
    FStringHash ContextHash;
    char ContextName[MAX_CONTEXT_NAME];
    bool bIsActive = true;
    int Priority = 0;

private:
    // 액션 매핑 설정
    void MapAction(const UInputAction& Action)
    {
        const FStringHash& actionHash = Action.GetHash();

        // 기존 매핑 제거 
        auto existingIt = ActionMappings.find(actionHash);
        if (existingIt != ActionMappings.end())
        {
            const auto& oldKeyCodes = existingIt->second.KeyCodes;

            // 역방향 매핑에서 제거
            for (WPARAM keyCode : oldKeyCodes)
            {
                auto& actions = KeyToActionMap[keyCode];
                actions.erase(std::remove(actions.begin(), actions.end(), actionHash), actions.end());
                if (actions.empty())
                {
                    KeyToActionMap.erase(keyCode);
                }
            }
        }

        // 새 매핑 추가
        ActionMappings[actionHash] = Action;

        // 키코드-액션 역방향 매핑 구축
        for (WPARAM keyCode : Action.KeyCodes)
        {
            KeyToActionMap[keyCode].push_back(actionHash);
        }
    }

public:
    // 생성자
    UInputContext(const char* Name, int InPriority = 0)
        : ContextHash(Name), Priority(InPriority)
    {
        if (Name)
        {
            strncpy_s(ContextName, Name, MAX_CONTEXT_NAME - 1);
            ContextName[MAX_CONTEXT_NAME - 1] = '\0';
        }
        else
        {
            ContextName[0] = '\0';
        }
    }

    ~UInputContext()
    {
        ClearAllBindings();
    }

    // 정적 생성 함수
    static std::shared_ptr<UInputContext> Create(const char* Name, int Priority = 0)
    {
        return std::make_shared<UInputContext>(Name, Priority);
    }

 
public:
    // 액션 바인딩
    template<typename T>
    void BindAction(const UInputAction& Action,
                    EKeyEvent EventType,
                    const std::weak_ptr<T>& InObject,
                    const std::function<void(const FKeyEventData&)>& InFunction,
                    const char* InFunctionName)
    {
        const FStringHash& actionHash = Action.GetHash();

        // 액션이 매핑되어 있지 않으면 자동으로 추가
        if (ActionMappings.find(actionHash) == ActionMappings.end())
        {
            MapAction(Action);
        }

        ActionEventDelegates[EventType][actionHash].Bind(InObject.lock().get(), InFunction, InFunctionName);
    }

    // 시스템 액션 바인딩
    void BindActionSystem(const UInputAction& Action,
                          EKeyEvent EventType,
                          const std::function<void(const FKeyEventData&)>& InFunction,
                          const char* InFunctionName)
    {
        const FStringHash& actionHash = Action.GetHash();

        // 액션이 매핑되어 있지 않으면 자동으로 추가
        if (ActionMappings.find(actionHash) == ActionMappings.end())
        {
            MapAction(Action);
        }

        ActionEventDelegates[EventType][actionHash].BindSystem(InFunction, InFunctionName);
    }

  
public:
    // 입력 처리 - 액션 기반만 담당
    bool ProcessInput(const FKeyEventData& EventData)
    {
        if (!bIsActive) return false;

        bool bHandled = false;

        // 액션 기반 처리
        auto keyActionsIt = KeyToActionMap.find(EventData.KeyCode);
        if (keyActionsIt != KeyToActionMap.end())
        {
            const auto& actionHashes = keyActionsIt->second;

            auto eventDelegatesIt = ActionEventDelegates.find(EventData.EventType);
            if (eventDelegatesIt != ActionEventDelegates.end())
            {
                const auto& eventMap = eventDelegatesIt->second;

                for (const FStringHash& actionHash : actionHashes)
                {
                    auto delegateIt = eventMap.find(actionHash);
                    if (delegateIt != eventMap.end())
                    {
                        delegateIt->second.Broadcast(EventData);
                        bHandled = true;
                    }
                }
            }
        }

        return bHandled;
    }

  
public:
    // 컨텍스트 관리
    void SetActive(bool bActive) { bIsActive = bActive; }
    bool IsActive() const { return bIsActive; }

    int GetPriority() const { return Priority; }
    void SetPriority(int NewPriority) { Priority = NewPriority; }

    const char* GetName() const { return ContextName; }
    const FStringHash& GetHash() const { return ContextHash; }

public:
    // 바인딩 해제 및 정리
    void ClearAllBindings()
    {
        ActionMappings.clear();
        ActionEventDelegates.clear();
        KeyToActionMap.clear();
    }

    void UnbindAction(const FStringHash& ActionHash, EKeyEvent EventType)
    {
        auto eventIt = ActionEventDelegates.find(EventType);
        if (eventIt != ActionEventDelegates.end())
        {
            eventIt->second.erase(ActionHash);
        }
    }

    void UnbindAction(const UInputAction& Action, EKeyEvent EventType)
    {
        UnbindAction(Action.GetHash(), EventType);
    }


public:
    // 디버깅 지원
    void PrintDebugInfo() const
    {
        LOG("InputContext: %s (Hash: %llu)", ContextName, ContextHash.GetHash());
        LOG("Registered Actions:");

        for (const auto& pair : ActionMappings)
        {
            const FStringHash& hash = pair.first;
            const UInputAction& action = pair.second;

            LOG("  - %s (Hash: %llu) -> Keys: ", action.GetName(), hash.GetHash());
            for (WPARAM key : action.KeyCodes)
            {
                LOG("    Key: %c (%d)", (char)key, (int)key);
            }
        }
    }
};