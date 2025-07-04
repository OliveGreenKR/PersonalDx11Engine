#pragma once
#include "PhysicsStateInternalInterface.h"
#include "Math.h"
#include "Transform.h"
#include "PhysicsDefine.h"

/// <summary>
/// 물리 Job 시스템
/// 
/// 설계 특징:
/// - IPhysicsStateInternal 인터페이스를 통해 시스템과 소통
/// - PhysicsSystem에 대한 직접적 의존성 제거
/// - 각 Job은 필요한 데이터만 저장하고 실행 시 인터페이스에 위임
/// </summary>
struct FPhysicsJob
{
    PhysicsID TargetID = 0;

    FPhysicsJob(PhysicsID InID) : TargetID(InID) {}
    virtual ~FPhysicsJob() = default;

    virtual void Execute(IPhysicsStateInternal* physicsInternal) = 0;
};

// === 구체적 Job 구현들 ===

// === 속도 관련 Job들 ===

struct FJobSetVelocity : public FPhysicsJob
{
    Vector3 Velocity;

    FJobSetVelocity(PhysicsID targetID, const Vector3& velocity)
        : FPhysicsJob(targetID)
        , Velocity(velocity)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetVelocity(TargetID, Velocity);
    }
};

struct FJobAddVelocity : public FPhysicsJob
{
    Vector3 DeltaVelocity;

    FJobAddVelocity(PhysicsID targetID, const Vector3& deltaVelocity)
        : FPhysicsJob(targetID)
        , DeltaVelocity(deltaVelocity)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_AddVelocity(TargetID, DeltaVelocity);
    }
};

struct FJobSetAngularVelocity : public FPhysicsJob
{
    Vector3 AngularVelocity;

    FJobSetAngularVelocity(PhysicsID targetID, const Vector3& angularVelocity)
        : FPhysicsJob(targetID)
        , AngularVelocity(angularVelocity)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetAngularVelocity(TargetID, AngularVelocity);
    }
};

struct FJobAddAngularVelocity : public FPhysicsJob
{
    Vector3 DeltaAngularVelocity;

    FJobAddAngularVelocity(PhysicsID targetID, const Vector3& deltaAngularVelocity)
        : FPhysicsJob(targetID)
        , DeltaAngularVelocity(deltaAngularVelocity)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_AddAngularVelocity(TargetID, DeltaAngularVelocity);
    }
};

// === 힘/충격 관련 Job들 ===

struct FJobApplyForce : public FPhysicsJob
{
    Vector3 Force;
    Vector3 Location;

    FJobApplyForce(PhysicsID targetID, const Vector3& force, const Vector3& location)
        : FPhysicsJob(targetID)
        , Force(force)
        , Location(location)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_ApplyForce(TargetID, Force, Location);
    }
};

struct FJobApplyImpulse : public FPhysicsJob
{
    Vector3 Impulse;
    Vector3 Location;

    FJobApplyImpulse(PhysicsID targetID, const Vector3& impulse, const Vector3& location)
        : FPhysicsJob(targetID)
        , Impulse(impulse)
        , Location(location)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_ApplyImpulse(TargetID, Impulse, Location);
    }
};
