#include "CollisionEventDispatcher.h"
#include "CollisionComponent.h"

FCollisionPair FCollisionEventDispatcher::CreateCollisionKey(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB) const
{
    // 포인터 주소를 기준으로 항상 작은 주소가 ComponentA가 되도록 정규화
    if (ComponentA.get() < ComponentB.get())
    {
        return FCollisionPair{ ComponentA, ComponentB };
    }
    return FCollisionPair{ ComponentB, ComponentA };
}

void FCollisionEventDispatcher::RegisterCollision(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB,
    const FCollisionEventData& EventData)
{
    // 둘 중 하나라도 충돌이 비활성화되어 있다면 무시
    if (!ComponentA->bCollisionEnabled || !ComponentB->bCollisionEnabled)
    {
        return;
    }

    // 정규화된 키 생성
    FCollisionPair Key = CreateCollisionKey(ComponentA, ComponentB);

    // 현재 프레임의 충돌 상태 업데이트
    auto& CurrentState = CurrentCollisionsPair[Key];

    // 이전 프레임에서의 상태 확인
    auto PrevIt = PreviousCollisionsPair.find(Key);
    bool WasColliding = (PrevIt != PreviousCollisionsPair.end());

    // 상태 업데이트
    if (WasColliding)
    {
        CurrentState.State = ECollisionState::Stay;
    }
    else
    {
        CurrentState.State = ECollisionState::Enter;
    }

    CurrentState.EventData = EventData;
}

void FCollisionEventDispatcher::UpdateCollisionStates()
{
    // 이전 프레임 데이터를 현재 프레임 데이터로 교체
    PreviousCollisionsPair = std::move(CurrentCollisionsPair);

    // 현재 프레임 데이터 초기화
    CurrentCollisionsPair.clear();

    // 메모리 예약으로 재할당 최소화
    CurrentCollisionsPair.reserve(PreviousCollisionsPair.size());
}

void FCollisionEventDispatcher::DispatchEvents()
{
    // 현재 프레임의 모든 충돌에 대해 Enter/Stay 이벤트 발행
    for (const auto& Pair : CurrentCollisionsPair)
    {
        if (!AreComponentsValid(Pair.first)) continue;

        auto ComponentA = Pair.first.ComponentA.lock();
        auto ComponentB = Pair.first.ComponentB.lock();

        switch (Pair.second.State)
        {
            case ECollisionState::Enter:
                DispatchCollisionEnter(ComponentA, ComponentB, Pair.second.EventData);
                break;
            case ECollisionState::Stay:
                DispatchCollisionStay(ComponentA, ComponentB, Pair.second.EventData);
                break;
        }
    }

    // 이전 프레임에서 충돌했지만 현재 프레임에서 충돌하지 않는 쌍에 대해 Exit 이벤트 발행
    for (const auto& Pair : PreviousCollisionsPair)
    {
        if (!AreComponentsValid(Pair.first)) continue;

        auto Key = Pair.first;
        if (CurrentCollisionsPair.find(Key) == CurrentCollisionsPair.end())
        {
            auto ComponentA = Key.ComponentA.lock();
            auto ComponentB = Key.ComponentB.lock();
            DispatchCollisionExit(ComponentA, ComponentB, Pair.second.EventData);
        }
    }
}

void FCollisionEventDispatcher::DispatchCollisionEnter(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB,
    const FCollisionEventData& EventData)
{
    // A -> B 이벤트 발행
    FCollisionEventData EventDataAB = EventData;
    EventDataAB.OtherComponent = ComponentB;
    ComponentA->OnCollisionEnterEvent(EventDataAB);

    // B -> A 이벤트 발행
    FCollisionEventData EventDataBA = EventData;
    EventDataBA.OtherComponent = ComponentA;
    // 충돌 노멀 방향 반전
    EventDataBA.CollisionResult.Normal = -EventData.CollisionResult.Normal;
    ComponentB->OnCollisionEnterEvent(EventDataBA);
}

void FCollisionEventDispatcher::DispatchCollisionStay(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB,
    const FCollisionEventData& EventData)
{
    // A -> B 이벤트 발행
    FCollisionEventData EventDataAB = EventData;
    EventDataAB.OtherComponent = ComponentB;
    ComponentA->OnCollisionStayEvent(EventDataAB);

    // B -> A 이벤트 발행
    FCollisionEventData EventDataBA = EventData;
    EventDataBA.OtherComponent = ComponentA;
    // 충돌 노멀 방향 반전
    EventDataBA.CollisionResult.Normal = -EventData.CollisionResult.Normal;
    ComponentB->OnCollisionStayEvent(EventDataBA);
}

void FCollisionEventDispatcher::DispatchCollisionExit(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB,
    const FCollisionEventData& EventData)
{
    // A -> B 이벤트 발행
    FCollisionEventData EventDataAB = EventData;
    EventDataAB.OtherComponent = ComponentB;
    ComponentA->OnCollisionExitEvent(EventDataAB);

    // B -> A 이벤트 발행
    FCollisionEventData EventDataBA = EventData;
    EventDataBA.OtherComponent = ComponentA;
    // 충돌 노멀 방향 반전
    EventDataBA.CollisionResult.Normal = -EventData.CollisionResult.Normal;
    ComponentB->OnCollisionExitEvent(EventDataBA);
}


bool FCollisionEventDispatcher::AreComponentsValid(const FCollisionPair& Key) const
{
    auto CompA = Key.ComponentA.lock();
    auto CompB = Key.ComponentB.lock();

    return CompA && CompB &&
        CompA->bCollisionEnabled &&
        CompB->bCollisionEnabled;
}


void FCollisionEventDispatcher::DestroyComponent(
    const std::shared_ptr<UCollisionComponent>& Component)
{
    // 컴포넌트를 destroyed로 마킹
    Component->bDestroyed = true;

    // 현재 프레임의 충돌 상태에서 관련된 모든 컴포넌트를 destroyed로 마킹
    for (const auto& Pair : CurrentCollisionsPair)
    {
        const auto& Key = Pair.first;
        auto CompA = Key.ComponentA.lock();
        auto CompB = Key.ComponentB.lock();

        if (CompA && CompA.get() == Component.get())
        {
            if (CompB) CompB->bDestroyed = true;
        }
        else if (CompB && CompB.get() == Component.get())
        {
            if (CompA) CompA->bDestroyed = true;
        }
    }

    // 이전 프레임의 충돌 상태에서도 동일하게 처리
    for (const auto& Pair : PreviousCollisionsPair)
    {
        const auto& Key = Pair.first;
        auto CompA = Key.ComponentA.lock();
        auto CompB = Key.ComponentB.lock();

        if (CompA && CompA.get() == Component.get())
        {
            if (CompB) CompB->bDestroyed = true;
        }
        else if (CompB && CompB.get() == Component.get())
        {
            if (CompA) CompA->bDestroyed = true;
        }
    }
}

void FCollisionEventDispatcher::RemoveDestroyedComponents()
{
    // 현재 프레임 충돌 정보 정리
    {
        std::unordered_map<FCollisionPair, FCollisionState> ValidCollisions;
        ValidCollisions.reserve(CurrentCollisionsPair.size());

        for (const auto& Pair : CurrentCollisionsPair)
        {
            const auto& Key = Pair.first;
            auto CompA = Key.ComponentA.lock();
            auto CompB = Key.ComponentB.lock();

            if (CompA && CompB && !CompA->bDestroyed && !CompB->bDestroyed)
            {
                ValidCollisions.emplace(Key, Pair.second);
            }
        }

        CurrentCollisionsPair.swap(ValidCollisions);
    }

    // 이전 프레임 충돌 정보 정리
    {
        std::unordered_map<FCollisionPair, FCollisionState> ValidCollisions;
        ValidCollisions.reserve(PreviousCollisionsPair.size());

        for (const auto& Pair : PreviousCollisionsPair)
        {
            const auto& Key = Pair.first;
            auto CompA = Key.ComponentA.lock();
            auto CompB = Key.ComponentB.lock();

            if (CompA && CompB && !CompA->bDestroyed && !CompB->bDestroyed)
            {
                ValidCollisions.emplace(Key, Pair.second);
            }
        }

        PreviousCollisionsPair.swap(ValidCollisions);
    }
}