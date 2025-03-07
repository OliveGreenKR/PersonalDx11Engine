#include "CollisionEventDispatcher.h"
#include "CollisionComponent.h"

void FCollisionEventDispatcher::DispatchCollisionEvents(const std::shared_ptr<UCollisionComponent>& InComponent, const FCollisionEventData& EventData, const ECollisionState& CollisionState)
{
    if (!InComponent.get() || !InComponent->IsCollisionEnabled())
    {
        return;
    }
    DispatchCollisionEvents(InComponent.get(), EventData, CollisionState);
}

void FCollisionEventDispatcher::DispatchCollisionEvents(const UCollisionComponent* InComponent, const FCollisionEventData& EventData, const ECollisionState& CollisionState)
{
    if (!InComponent || !InComponent->IsCollisionEnabled())
    {
        return;
    }

    auto OtherComponent = EventData.OtherComponent.lock();
    if (!OtherComponent || !OtherComponent->IsCollisionEnabled())
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
