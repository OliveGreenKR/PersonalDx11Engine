#include "CollisionEventDispatcher.h"
#include "CollisionComponent.h"

FCollisionPair FCollisionEventDispatcher::CreateCollisionKey(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB) const
{
    // ������ �ּҸ� �������� �׻� ���� �ּҰ� ComponentA�� �ǵ��� ����ȭ
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
    // �� �� �ϳ��� �浹�� ��Ȱ��ȭ�Ǿ� �ִٸ� ����
    if (!ComponentA->bCollisionEnabled || !ComponentB->bCollisionEnabled)
    {
        return;
    }

    // ����ȭ�� Ű ����
    FCollisionPair Key = CreateCollisionKey(ComponentA, ComponentB);

    // ���� �������� �浹 ���� ������Ʈ
    auto& CurrentState = CurrentCollisionsPair[Key];

    // ���� �����ӿ����� ���� Ȯ��
    auto PrevIt = PreviousCollisionsPair.find(Key);
    bool WasColliding = (PrevIt != PreviousCollisionsPair.end());

    // ���� ������Ʈ
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
    // ���� ������ �����͸� ���� ������ �����ͷ� ��ü
    PreviousCollisionsPair = std::move(CurrentCollisionsPair);

    // ���� ������ ������ �ʱ�ȭ
    CurrentCollisionsPair.clear();

    // �޸� �������� ���Ҵ� �ּ�ȭ
    CurrentCollisionsPair.reserve(PreviousCollisionsPair.size());
}

void FCollisionEventDispatcher::DispatchEvents()
{
    // ���� �������� ��� �浹�� ���� Enter/Stay �̺�Ʈ ����
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

    // ���� �����ӿ��� �浹������ ���� �����ӿ��� �浹���� �ʴ� �ֿ� ���� Exit �̺�Ʈ ����
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
    // A -> B �̺�Ʈ ����
    FCollisionEventData EventDataAB = EventData;
    EventDataAB.OtherComponent = ComponentB;
    ComponentA->OnCollisionEnterEvent(EventDataAB);

    // B -> A �̺�Ʈ ����
    FCollisionEventData EventDataBA = EventData;
    EventDataBA.OtherComponent = ComponentA;
    // �浹 ��� ���� ����
    EventDataBA.CollisionResult.Normal = -EventData.CollisionResult.Normal;
    ComponentB->OnCollisionEnterEvent(EventDataBA);
}

void FCollisionEventDispatcher::DispatchCollisionStay(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB,
    const FCollisionEventData& EventData)
{
    // A -> B �̺�Ʈ ����
    FCollisionEventData EventDataAB = EventData;
    EventDataAB.OtherComponent = ComponentB;
    ComponentA->OnCollisionStayEvent(EventDataAB);

    // B -> A �̺�Ʈ ����
    FCollisionEventData EventDataBA = EventData;
    EventDataBA.OtherComponent = ComponentA;
    // �浹 ��� ���� ����
    EventDataBA.CollisionResult.Normal = -EventData.CollisionResult.Normal;
    ComponentB->OnCollisionStayEvent(EventDataBA);
}

void FCollisionEventDispatcher::DispatchCollisionExit(
    const std::shared_ptr<UCollisionComponent>& ComponentA,
    const std::shared_ptr<UCollisionComponent>& ComponentB,
    const FCollisionEventData& EventData)
{
    // A -> B �̺�Ʈ ����
    FCollisionEventData EventDataAB = EventData;
    EventDataAB.OtherComponent = ComponentB;
    ComponentA->OnCollisionExitEvent(EventDataAB);

    // B -> A �̺�Ʈ ����
    FCollisionEventData EventDataBA = EventData;
    EventDataBA.OtherComponent = ComponentA;
    // �浹 ��� ���� ����
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
    // ������Ʈ�� destroyed�� ��ŷ
    Component->bDestroyed = true;

    // ���� �������� �浹 ���¿��� ���õ� ��� ������Ʈ�� destroyed�� ��ŷ
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

    // ���� �������� �浹 ���¿����� �����ϰ� ó��
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
    // ���� ������ �浹 ���� ����
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

    // ���� ������ �浹 ���� ����
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