#pragma once
#include <unordered_map>
#include <vector>
#include "CollisionDefines.h"

class UCollisionComponent;

class FCollisionEventDispatcher
{
public:
    FCollisionEventDispatcher() = default;
    ~FCollisionEventDispatcher() = default;

public:
    void DispatchCollisionEvents(
        const std::shared_ptr<UCollisionComponent>& InComponent,
        const FCollisionEventData& EventData,
        const ECollisionState& CollisionState);
};