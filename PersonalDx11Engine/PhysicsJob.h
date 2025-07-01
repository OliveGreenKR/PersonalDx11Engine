#pragma once
#include "Math.h"
#include "Transform.h"
#include "PhysicsDefine.h"
#include "PhysicsStateInternalInterface.h"

// === 속도 관련 Job들 ===
struct FPhysicsJob
{
    FPhysicsJob(SoAID InID) : TargetID(InID) {}
    virtual ~FPhysicsJob() = default;

    SoAID TargetID = 0;

    virtual void Execute(SoAID targetID, IPhysicsStateInternal* PhysicsStates) = 0;
};
