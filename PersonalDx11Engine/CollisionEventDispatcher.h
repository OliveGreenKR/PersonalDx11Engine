#pragma once
#include <unordered_map>
#include <vector>
#include "CollisionDefines.h"

class UCollisionComponentBase;

class FCollisionEventDispatcher
{
    friend class UCollisionManager;

public:
    FCollisionEventDispatcher() = default;
    ~FCollisionEventDispatcher() = default;

public:
    void DispatchCollisionEvents(
        const std::shared_ptr<UCollisionComponentBase>& InComponent,
        const FCollisionEventData& EventData,
        const ECollisionState& CollisionState);
private:
    void DispatchCollisionEvents(
        const UCollisionComponentBase* InComponent,
        const FCollisionEventData& EventData,
        const ECollisionState& CollisionState);

};