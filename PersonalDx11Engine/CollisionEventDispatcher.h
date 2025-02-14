#pragma once
#include <unordered_map>
#include <vector>
#include "CollisionDefines.h"

class UCollisionComponent;

class FCollisionEventDispatcher
{
    friend class UCollisionManager;

public:
    FCollisionEventDispatcher() = default;
    ~FCollisionEventDispatcher() = default;

public:
    void DispatchCollisionEvents(
        const std::shared_ptr<UCollisionComponent>& InComponent,
        const FCollisionEventData& EventData,
        const ECollisionState& CollisionState);
private:
    void DispatchCollisionEvents(
        const UCollisionComponent* InComponent,
        const FCollisionEventData& EventData,
        const ECollisionState& CollisionState);

};