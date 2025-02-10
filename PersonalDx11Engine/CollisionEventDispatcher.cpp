#include "CollisionEventDispatcher.h"
#include "CollisionComponent.h"

void FCollisionEventDispatcher::DispatchCollisionEvents(const std::shared_ptr<UCollisionComponent>& InComponent, const FCollisionEventData& EventData, const ECollisionState& CollisionState)
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

    // 중복 방지를 위해 단방향 이벤트만 발생
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
