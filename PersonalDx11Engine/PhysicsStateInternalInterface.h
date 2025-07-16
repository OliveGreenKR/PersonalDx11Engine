#pragma once
#include "Math.h"
#include "Transform.h"
#include "PhysicsDefine.h"

using PhysicsID = std::uint32_t;
/// <summary>
/// 물리 시뮬레이션 내부에서 사용하는 물리 상태 인터페이스 (SoA 기반)
/// 
/// 책임:
/// - 중앙화된 물리 상태 데이터에 대한 CRUD 인터페이스 제공
/// - Job 시스템이 PhysicsStateSoA를 조작할 수 있는 추상화 레이어
/// - 물리 시뮬레이션 내부에서만 사용 (즉각 데이터 수정 가능)
/// </summary>
class IPhysicsStateInternal
{
public:
    virtual ~IPhysicsStateInternal() = default;

#pragma region Physical Properties Access (기존 기능 유지)

    // === 물리 속성 접근자 (PhysicsID 기반) ===
    virtual float P_GetMass(PhysicsID targetID) const = 0;
    virtual float P_GetInvMass(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetRotationalInertia(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetInvRotationalInertia(PhysicsID targetID) const = 0;
    virtual float P_GetRestitution(PhysicsID targetID) const = 0;
    virtual float P_GetFrictionStatic(PhysicsID targetID) const = 0;
    virtual float P_GetFrictionKinetic(PhysicsID targetID) const = 0;
    virtual float P_GetGravityScale(PhysicsID targetID) const = 0;
    virtual float P_GetMaxSpeed(PhysicsID targetID) const = 0;
    virtual float P_GetMaxAngularSpeed(PhysicsID targetID) const = 0;
#pragma endregion

#pragma region Motion State Access (기존 기능 유지)

    // === 운동 상태 접근자 (XMVECTOR 가상 함수) ===
    virtual XMVECTOR P_GetVelocity(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetAngularVelocity(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetAccumulatedForce(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetAccumulatedTorque(PhysicsID targetID) const = 0;

#pragma endregion

#pragma region Transform Access (기존 기능 유지)

    // === 트랜스폼 접근자 (XMVECTOR 가상 함수) ===
    virtual XMVECTOR P_GetWorldPosition(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetWorldRotationQuat(PhysicsID targetID) const = 0;
    virtual XMVECTOR P_GetWorldScale(PhysicsID targetID) const = 0;
    virtual XMMATRIX P_GetWorldTransformMatrix(PhysicsID targetID) const = 0;

#pragma endregion

#pragma region Force and Impulse Application

    // === 힘/충격 적용 (XMVECTOR 가상 함수) ===
    virtual void P_ApplyForce(PhysicsID targetID, XMVECTOR force, XMVECTOR location) = 0;
    virtual void P_ApplyImpulse(PhysicsID targetID, XMVECTOR impulse, XMVECTOR location) = 0;

    // === 기존 Vector3 버전 (일반 함수 오버로드) ===
    void P_ApplyForce(PhysicsID targetID, const Vector3& force)
    {
        XMVECTOR centerOfMass = P_GetWorldPosition(targetID);
        XMVECTOR forceVec = XMVectorSet(force.x, force.y, force.z, 0.0f);
        P_ApplyForce(targetID, forceVec, centerOfMass);
    }

    void P_ApplyForce(PhysicsID targetID, const Vector3& force, const Vector3& location)
    {
        XMVECTOR forceVec = XMVectorSet(force.x, force.y, force.z, 0.0f);
        XMVECTOR locationVec = XMVectorSet(location.x, location.y, location.z, 1.0f);
        P_ApplyForce(targetID, forceVec, locationVec);
    }

    void P_ApplyImpulse(PhysicsID targetID, const Vector3& impulse)
    {
        XMVECTOR centerOfMass = P_GetWorldPosition(targetID);
        XMVECTOR impulseVec = XMVectorSet(impulse.x, impulse.y, impulse.z, 0.0f);
        P_ApplyImpulse(targetID, impulseVec, centerOfMass);
    }

    void P_ApplyImpulse(PhysicsID targetID, const Vector3& impulse, const Vector3& location)
    {
        XMVECTOR impulseVec = XMVectorSet(impulse.x, impulse.y, impulse.z, 0.0f);
        XMVECTOR locationVec = XMVectorSet(location.x, location.y, location.z, 1.0f);
        P_ApplyImpulse(targetID, impulseVec, locationVec);
    }

#pragma endregion

#pragma region Property Setters (기존 기능 유지)

    // === 물리 속성 설정자 (기존 기능 유지) ===
    virtual void P_SetMass(PhysicsID targetID, float mass) = 0;
    virtual void P_SetInvMass(PhysicsID targetID, float invMass) = 0;
    virtual void P_SetRestitution(PhysicsID targetID, float restitution) = 0;
    virtual void P_SetFrictionStatic(PhysicsID targetID, float frictionStatic) = 0;
    virtual void P_SetFrictionKinetic(PhysicsID targetID, float frictionKinetic) = 0;
    virtual void P_SetGravityScale(PhysicsID targetID, float gravityScale) = 0;
    virtual void P_SetMaxSpeed(PhysicsID targetID, float maxSpeed) = 0;
    virtual void P_SetMaxAngularSpeed(PhysicsID targetID, float maxAngularSpeed) = 0;

    // === 벡터 속성 설정 (XMVECTOR 가상 함수) ===
    virtual void P_SetRotationalInertia(PhysicsID targetID, XMVECTOR rotationalInertia) = 0;
    virtual void P_SetInvRotationalInertia(PhysicsID targetID, XMVECTOR invRotationalInertia) = 0;

    // === 벡터 속성 설정 (Vector3 오버로드) ===
    void P_SetRotationalInertia(PhysicsID targetID, const Vector3& rotationalInertia)
    {
        XMVECTOR inertiaVec = XMVectorSet(rotationalInertia.x, rotationalInertia.y, rotationalInertia.z, 0.0f);
        P_SetRotationalInertia(targetID, inertiaVec);
    }

    void P_SetInvRotationalInertia(PhysicsID targetID, const Vector3& invRotationalInertia)
    {
        XMVECTOR invInertiaVec = XMVectorSet(invRotationalInertia.x, invRotationalInertia.y, invRotationalInertia.z, 0.0f);
        P_SetInvRotationalInertia(targetID, invInertiaVec);
    }

#pragma endregion

#pragma region State Type and Control (기존 기능 유지)

    // === 상태 타입 및 마스크 설정자 ===
    virtual void P_SetPhysicsType(PhysicsID targetID, EPhysicsType physicsType) = 0;
    virtual void P_SetPhysicsMask(PhysicsID targetID, const FPhysicsMask& physicsMask) = 0;

    // === 활성화 제어 ===
    virtual void P_SetPhysicsActive(PhysicsID targetID, bool bActive) = 0;
    virtual bool P_IsPhysicsActive(PhysicsID targetID) const = 0;

#pragma endregion
};