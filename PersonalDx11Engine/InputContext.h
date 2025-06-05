#pragma once
#include "InputDefines.h"

class UInputContext : public std::enable_shared_from_this<UInputContext>
{
private:
    // 키 이벤트 타입별 델리게이트 맵
    std::unordered_map<EKeyEvent, FOnKeyEvent> KeyEventDelegates;

    // 액션 이름과 액션 매핑 정보
    std::unordered_map<std::string, UInputAction> ActionMappings;

    // 액션 이벤트를 위한 델리게이트 맵 (키 타입별로 액션별 델리게이트 관리)
    std::unordered_map<EKeyEvent, std::unordered_map<std::string, FOnKeyEvent>> ActionEventDelegates;

    // 키코드에서 액션 이름으로의 역방향 맵핑 (키 코드 -> 액션 이름 목록)
    std::unordered_map<WPARAM, std::vector<std::string>> KeyToActionMap;

    bool bIsActive = true;
    std::string ContextName;
    int Priority = 0;

    // 액션 매핑 설정 
    void MapAction(const UInputAction& Action)
    {
        // 기존 매핑 제거 
        auto existingIt = ActionMappings.find(Action.GetName()); 
        if (existingIt != ActionMappings.end()) {
            const auto& oldKeyCodes = existingIt->second.KeyCodes; 

            for (WPARAM keyCode : oldKeyCodes) {
                auto& actions = KeyToActionMap[keyCode];

                actions.erase(std::remove(actions.begin(), actions.end(), Action.GetName()), actions.end());
                if (actions.empty()) {
                    KeyToActionMap.erase(keyCode);
                }
            }
        }

        // 새 매핑 추가
        ActionMappings[Action.GetName()] = Action;

        // 키코드-액션 역방향 매핑 구축
        for (WPARAM keyCode : Action.KeyCodes) {
            KeyToActionMap[keyCode].push_back(Action.GetName());
        }
    }

public:
    UInputContext(const std::string& Name, int InPriority = 0)
        : ContextName(Name), Priority(InPriority)
    {
    }

public:
    ~UInputContext()
    {
        ClearAllBindings();
    }

    static std::shared_ptr<UInputContext> Create(const std::string& Name, int Priority = 0)
    {

        // make_shared 로 효율적인 한번의 할당만
        return std::make_shared<UInputContext>(Name, Priority);
    }


    // 액션 바인딩 (객체를 사용하는 버전) - Action 객체 직접 사용
    template<typename T>
    void BindAction(const UInputAction& Action,
                    EKeyEvent EventType,
                    const std::weak_ptr<T>& InObject,
                    const std::function<void(const FKeyEventData&)>& InFunction,
                    const std::string& InFunctionName)
    {
        std::string ActionName = Action.GetName();
        // 액션이 매핑되어 있지 않으면 자동으로 추가
        if (ActionMappings.find(ActionName) == ActionMappings.end()) {
            MapAction(Action);
        }

        ActionEventDelegates[EventType][ActionName].Bind(InObject.lock().get(), InFunction, InFunctionName);
    }

    // 액션 바인딩 (객체 없이 사용) - Action 객체 직접 사용
    void BindActionSystem(const UInputAction& Action,
                          EKeyEvent EventType,
                          const std::function<void(const FKeyEventData&)>& InFunction,
                          const std::string& InFunctionName)
    {
        std::string ActionName = Action.GetName();
        // 액션이 매핑되어 있지 않으면 자동으로 추가
        if (ActionMappings.find(ActionName) == ActionMappings.end()) {
            MapAction(Action);
        }

        ActionEventDelegates[EventType][ActionName].BindSystem(InFunction, InFunctionName);
    }


    // 키 이벤트 시스템 바인딩
    void BindKeyEventSystem(EKeyEvent EventType,
                            const std::function<void(const FKeyEventData&)>& InFunction,
                            const std::string& InFunctionName)
    {
        KeyEventDelegates[EventType].BindSystem(InFunction, InFunctionName);
    }

    // 특정 액션 언바인딩 - Action 객체 직접 사용
    template<typename T>
    void UnbindAction(const UInputAction& Action,
                      EKeyEvent EventType,
                      const std::weak_ptr<T>& InObject,
                      const std::string& InFunctionName)
    {
        std::string ActionName = Action.GetName();
        auto it = ActionEventDelegates.find(EventType);
        if (it != ActionEventDelegates.end()) {
            auto actionIt = it->second.find(ActionName);
            if (actionIt != it->second.end()) {
                actionIt->second.Unbind(InObject, InFunctionName);
            }
        }
    }

    // 객체의 모든 바인딩 제거
    template<typename T>
    void UnbindAllForObject(const std::weak_ptr<T>& InObject)
    {
        // 일반 키 이벤트 언바인드
        for (auto& Delegate : KeyEventDelegates) {
            Delegate.second.UnbindAll(InObject);
        }

        // 액션 이벤트 언바인드
        for (auto& EventTypePair : ActionEventDelegates) {
            for (auto& ActionPair : EventTypePair.second) {
                ActionPair.second.UnbindAll(InObject);
            }
        }
    }

    // 모든 바인딩 제거
    void ClearAllBindings()
    {
        for (auto& Delegate : KeyEventDelegates) {
            Delegate.second.UnbindAll();
        }

        for (auto& EventTypePair : ActionEventDelegates) {
            for (auto& ActionPair : EventTypePair.second) {
                ActionPair.second.UnbindAll();
            }
        }
    }

    // 입력 처리
    bool ProcessInput(const FKeyEventData& EventData)
    {
        if (!bIsActive) return false;

        bool bHandled = false;

        // 1. 특정 키에 매핑된 액션 처리
        auto keyToActionIt = KeyToActionMap.find(EventData.KeyCode);
        if (keyToActionIt != KeyToActionMap.end()) {
            auto eventTypeIt = ActionEventDelegates.find(EventData.EventType);
            if (eventTypeIt != ActionEventDelegates.end()) {
                // 이 키에 매핑된 모든 액션에 대해 처리
                for (const std::string& actionName : keyToActionIt->second) {
                    auto actionIt = eventTypeIt->second.find(actionName);
                    if (actionIt != eventTypeIt->second.end()) {
                        actionIt->second.Broadcast(EventData);
                        bHandled = true;
                    }
                }
            }
        }

        // 2. 일반 키 이벤트 처리 (이전 로직 유지)
        auto it = KeyEventDelegates.find(EventData.EventType);
        if (it != KeyEventDelegates.end()) {
            it->second.Broadcast(EventData);
            bHandled = true;
        }

        return bHandled;
    }

    // 활성화/비활성화
    void SetActive(bool bActive) noexcept { bIsActive = bActive; }
    bool IsActive() const { return bIsActive; }

    // 우선순위 관리
    int GetPriority() const { return Priority; }
    void SetPriority(int NewPriority) noexcept { Priority = NewPriority; }

    // 컨텍스트 이름 반환
    const std::string& GetName() const { return ContextName; }
};