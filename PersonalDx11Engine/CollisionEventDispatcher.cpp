#include "CollisionEventDispatcher.h"
#include "CollisionComponent.h"

void FCollisionEventDispatcher::DispatchCollisionEvents(const std::shared_ptr<UCollisionComponent>& InComponent, const FCollisionEventData& EventData, const ECollisionState& CollisionState)
{
    if (!InComponent.get() || !InComponent->bCollisionEnabled)
    {
        return;
    }
    DispatchCollisionEvents(InComponent.get(), EventData, CollisionState);
}

void FCollisionEventDispatcher::DispatchCollisionEvents(const UCollisionComponent* InComponent, const FCollisionEventData& EventData, const ECollisionState& CollisionState)
{
    if (!InComponent || !InComponent->bCollisionEnabled)
    {
        return;
    }

    auto OtherComponent = EventData.OtherComponent.lock();
    if (!OtherComponent || !OtherComponent->bCollisionEnabled)
    {
        return;
    }

    // �ߺ� ������ ���� �ܹ��� �̺�Ʈ�� �߻�
    switch (CollisionState)
    {
        case ECollisionState::Enter:
            OtherComponent->OnCollisionEnterEvent(EventData);
            break;
        case ECollisionState::Stay:
            OtherComponent->OnCollisionStayEvent(EventData);
            break;
        case ECollisionState::Exit:
            OtherComponent->OnCollisionExitEvent(EventData);
            break;
    }
}
