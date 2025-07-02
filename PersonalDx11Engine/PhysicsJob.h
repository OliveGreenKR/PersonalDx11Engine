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
    SoAID TargetID = 0;

    FPhysicsJob(SoAID InID) : TargetID(InID) {}
    virtual ~FPhysicsJob() = default;

    virtual void Execute(IPhysicsStateInternal* physicsInternal) = 0;
};

// === 구체적 Job 구현들 ===

// === 속도 관련 Job들 ===

struct FJobSetVelocity : public FPhysicsJob
{
    Vector3 Velocity;

    FJobSetVelocity(SoAID targetID, const Vector3& velocity)
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

    FJobAddVelocity(SoAID targetID, const Vector3& deltaVelocity)
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

    FJobSetAngularVelocity(SoAID targetID, const Vector3& angularVelocity)
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

    FJobAddAngularVelocity(SoAID targetID, const Vector3& deltaAngularVelocity)
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

struct FJobApplyForceAtCenter : public FPhysicsJob
{
    Vector3 Force;

    FJobApplyForceAtCenter(SoAID targetID, const Vector3& force)
        : FPhysicsJob(targetID)
        , Force(force)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_ApplyForce(TargetID, Force);
    }
};

struct FJobApplyForceAtLocation : public FPhysicsJob
{
    Vector3 Force;
    Vector3 Location;

    FJobApplyForceAtLocation(SoAID targetID, const Vector3& force, const Vector3& location)
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

struct FJobApplyImpulseAtCenter : public FPhysicsJob
{
    Vector3 Impulse;

    FJobApplyImpulseAtCenter(SoAID targetID, const Vector3& impulse)
        : FPhysicsJob(targetID)
        , Impulse(impulse)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_ApplyImpulse(TargetID, Impulse);
    }
};

struct FJobApplyImpulseAtLocation : public FPhysicsJob
{
    Vector3 Impulse;
    Vector3 Location;

    FJobApplyImpulseAtLocation(SoAID targetID, const Vector3& impulse, const Vector3& location)
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

struct FJobApplyTorque : public FPhysicsJob
{
    Vector3 Torque;

    FJobApplyTorque(SoAID targetID, const Vector3& torque)
        : FPhysicsJob(targetID)
        , Torque(torque)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_ApplyTorque(TargetID, Torque);
    }
};

// === 트랜스폼 관련 Job들 ===

struct FJobSetWorldTransform : public FPhysicsJob
{
    FTransform WorldTransform;

    FJobSetWorldTransform(SoAID targetID, const FTransform& transform)
        : FPhysicsJob(targetID)
        , WorldTransform(transform)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetWorldTransform(TargetID, WorldTransform);
    }
};

struct FJobSetWorldPosition : public FPhysicsJob
{
    Vector3 Position;

    FJobSetWorldPosition(SoAID targetID, const Vector3& position)
        : FPhysicsJob(targetID)
        , Position(position)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetWorldPosition(TargetID, Position);
    }
};

struct FJobSetWorldRotation : public FPhysicsJob
{
    Quaternion Rotation;

    FJobSetWorldRotation(SoAID targetID, const Quaternion& rotation)
        : FPhysicsJob(targetID)
        , Rotation(rotation)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetWorldRotation(TargetID, Rotation);
    }
};

struct FJobSetWorldScale : public FPhysicsJob
{
    Vector3 Scale;

    FJobSetWorldScale(SoAID targetID, const Vector3& scale)
        : FPhysicsJob(targetID)
        , Scale(scale)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetWorldScale(TargetID, Scale);
    }
};

// === 물리 속성 관련 Job들 ===

struct FJobSetMass : public FPhysicsJob
{
    float Mass;

    FJobSetMass(SoAID targetID, float mass)
        : FPhysicsJob(targetID)
        , Mass(mass)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetMass(TargetID, Mass);
    }
};

struct FJobSetInvMass : public FPhysicsJob
{
    float InvMass;

    FJobSetInvMass(SoAID targetID, float invMass)
        : FPhysicsJob(targetID)
        , InvMass(invMass)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetInvMass(TargetID, InvMass);
    }
};

struct FJobSetRotationalInertia : public FPhysicsJob
{
    Vector3 RotationalInertia;

    FJobSetRotationalInertia(SoAID targetID, const Vector3& rotationalInertia)
        : FPhysicsJob(targetID)
        , RotationalInertia(rotationalInertia)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetRotationalInertia(TargetID, RotationalInertia);
    }
};

struct FJobSetInvRotationalInertia : public FPhysicsJob
{
    Vector3 InvRotationalInertia;

    FJobSetInvRotationalInertia(SoAID targetID, const Vector3& invRotationalInertia)
        : FPhysicsJob(targetID)
        , InvRotationalInertia(invRotationalInertia)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetInvRotationalInertia(TargetID, InvRotationalInertia);
    }
};

struct FJobSetFrictionKinetic : public FPhysicsJob
{
    float FrictionKinetic;

    FJobSetFrictionKinetic(SoAID targetID, float friction)
        : FPhysicsJob(targetID)
        , FrictionKinetic(friction)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetFrictionKinetic(TargetID, FrictionKinetic);
    }
};

struct FJobSetFrictionStatic : public FPhysicsJob
{
    float FrictionStatic;

    FJobSetFrictionStatic(SoAID targetID, float friction)
        : FPhysicsJob(targetID)
        , FrictionStatic(friction)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetFrictionStatic(TargetID, FrictionStatic);
    }
};

struct FJobSetRestitution : public FPhysicsJob
{
    float Restitution;

    FJobSetRestitution(SoAID targetID, float restitution)
        : FPhysicsJob(targetID)
        , Restitution(restitution)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetRestitution(TargetID, Restitution);
    }
};

struct FJobSetGravityScale : public FPhysicsJob
{
    float GravityScale;

    FJobSetGravityScale(SoAID targetID, float scale)
        : FPhysicsJob(targetID)
        , GravityScale(scale)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetGravityScale(TargetID, GravityScale);
    }
};

struct FJobSetMaxSpeed : public FPhysicsJob
{
    float MaxSpeed;

    FJobSetMaxSpeed(SoAID targetID, float maxSpeed)
        : FPhysicsJob(targetID)
        , MaxSpeed(maxSpeed)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetMaxSpeed(TargetID, MaxSpeed);
    }
};

struct FJobSetMaxAngularSpeed : public FPhysicsJob
{
    float MaxAngularSpeed;

    FJobSetMaxAngularSpeed(SoAID targetID, float maxAngularSpeed)
        : FPhysicsJob(targetID)
        , MaxAngularSpeed(maxAngularSpeed)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetMaxAngularSpeed(TargetID, MaxAngularSpeed);
    }
};

// === 상태 타입 및 마스크 관련 Job들 ===

struct FJobSetPhysicsType : public FPhysicsJob
{
    EPhysicsType PhysicsType;

    FJobSetPhysicsType(SoAID targetID, EPhysicsType physicsType)
        : FPhysicsJob(targetID)
        , PhysicsType(physicsType)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetPhysicsType(TargetID, PhysicsType);
    }
};

struct FJobSetPhysicsMask : public FPhysicsJob
{
    FPhysicsMask PhysicsMask;

    FJobSetPhysicsMask(SoAID targetID, const FPhysicsMask& physicsMask)
        : FPhysicsJob(targetID)
        , PhysicsMask(physicsMask)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetPhysicsMask(TargetID, PhysicsMask);
    }
};

// === 활성화 제어 관련 Job들 ===

struct FJobSetPhysicsActive : public FPhysicsJob
{
    bool bActive;

    FJobSetPhysicsActive(SoAID targetID, bool active)
        : FPhysicsJob(targetID)
        , bActive(active)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetPhysicsActive(TargetID, bActive);
    }
};

struct FJobSetCollisionActive : public FPhysicsJob
{
    bool bActive;

    FJobSetCollisionActive(SoAID targetID, bool active)
        : FPhysicsJob(targetID)
        , bActive(active)
    {
    }

    void Execute(IPhysicsStateInternal* physicsInternal) override
    {
        physicsInternal->P_SetCollisionActive(TargetID, bActive);
    }
};