#pragma once
#include "Math.h"
#include "Transform.h"
#include "PhysicsDefine.h"

using PhysicsID = std::uint32_t;
/// <summary>
/// 물리 시뮬레이션 내부에서 사용하는 물리 상태 인터페이스 (SoA 기반)
/// 
/// 설계 변경 사항:
/// - 기존: 단일 객체 조작 (P_SetVelocity(const Vector3&))
/// - 신규: PhysicsID 기반 중앙 관리 (P_SetVelocity(PhysicsID, const Vector3&))
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

    // === 물리 속성 접근자 (PhysicsID 기반) ===

    virtual float P_GetMass(PhysicsID targetID) const = 0;
    virtual float P_GetInvMass(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetRotationalInertia(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetInvRotationalInertia(PhysicsID targetID) const = 0;
    virtual float P_GetRestitution(PhysicsID targetID) const = 0;
    virtual float P_GetFrictionStatic(PhysicsID targetID) const = 0;
    virtual float P_GetFrictionKinetic(PhysicsID targetID) const = 0;
    virtual float P_GetGravityScale(PhysicsID targetID) const = 0;
    virtual float P_GetMaxSpeed(PhysicsID targetID) const = 0;
    virtual float P_GetMaxAngularSpeed(PhysicsID targetID) const = 0;

    // === 운동 상태 접근자 (PhysicsID 기반) ===

    virtual Vector3 P_GetVelocity(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetAngularVelocity(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetAccumulatedForce(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetAccumulatedTorque(PhysicsID targetID) const = 0;

    // === 트랜스폼 접근자 (PhysicsID 기반) ===

    virtual FTransform P_GetWorldTransform(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetWorldPosition(PhysicsID targetID) const = 0;
    virtual Quaternion P_GetWorldRotation(PhysicsID targetID) const = 0;
    virtual Vector3 P_GetWorldScale(PhysicsID targetID) const = 0;

    // === 상태 타입 및 마스크 접근자 ===

    virtual EPhysicsType P_GetPhysicsType(PhysicsID targetID) const = 0;
    virtual FPhysicsMask P_GetPhysicsMask(PhysicsID targetID) const = 0;

    // === 운동 상태 설정자 (PhysicsID 기반) ===

    virtual void P_SetVelocity(PhysicsID targetID, const Vector3& velocity) = 0;
    virtual void P_AddVelocity(PhysicsID targetID, const Vector3& deltaVelocity) = 0;
    virtual void P_SetAngularVelocity(PhysicsID targetID, const Vector3& angularVelocity) = 0;
    virtual void P_AddAngularVelocity(PhysicsID targetID, const Vector3& deltaAngularVelocity) = 0;

    // === 트랜스폼 설정자 (PhysicsID 기반) ===

    virtual void P_SetWorldPosition(PhysicsID targetID, const Vector3& position) = 0;
    virtual void P_SetWorldRotation(PhysicsID targetID, const Quaternion& rotation) = 0;
    virtual void P_SetWorldScale(PhysicsID targetID, const Vector3& scale) = 0;

    /// <summary>
    /// Transform 전체 설정 - 개별 설정 메서드들을 조합
    /// </summary>
    void P_SetWorldTransform(PhysicsID targetID, const FTransform& transform)
    {
        P_SetWorldPosition(targetID, transform.Position);
        P_SetWorldRotation(targetID, transform.Rotation);
        P_SetWorldScale(targetID, transform.Scale);
    }

    // === 힘/충격 적용 (PhysicsID 기반) ===

    void P_ApplyForce(PhysicsID targetID, const Vector3& force)
    {
        Vector3 centerOfMass = P_GetWorldPosition(targetID);
        P_ApplyForce(targetID, force, centerOfMass);
    }
    virtual void P_ApplyForce(PhysicsID targetID, const Vector3& force, const Vector3& location) = 0;
    void P_ApplyImpulse(PhysicsID targetID, const Vector3& impulse)
    {
        Vector3 centerOfMass = P_GetWorldPosition(targetID);
        P_ApplyImpulse(targetID, impulse, centerOfMass);
    }
    virtual void P_ApplyImpulse(PhysicsID targetID, const Vector3& impulse, const Vector3& location) = 0;

    // === 물리 속성 설정자 (PhysicsID 기반) ===

    virtual void P_SetMass(PhysicsID targetID, float mass) = 0;
    virtual void P_SetInvMass(PhysicsID targetID, float invMass) = 0;
    virtual void P_SetRotationalInertia(PhysicsID targetID, const Vector3& rotationalInertia) = 0;
    virtual void P_SetInvRotationalInertia(PhysicsID targetID, const Vector3& invRotationalInertia) = 0;
    virtual void P_SetRestitution(PhysicsID targetID, float restitution) = 0;
    virtual void P_SetFrictionStatic(PhysicsID targetID, float frictionStatic) = 0;
    virtual void P_SetFrictionKinetic(PhysicsID targetID, float frictionKinetic) = 0;
    virtual void P_SetGravityScale(PhysicsID targetID, float gravityScale) = 0;
    virtual void P_SetMaxSpeed(PhysicsID targetID, float maxSpeed) = 0;
    virtual void P_SetMaxAngularSpeed(PhysicsID targetID, float maxAngularSpeed) = 0;

    // === 상태 타입 및 마스크 설정자 ===

    virtual void P_SetPhysicsType(PhysicsID targetID, EPhysicsType physicsType) = 0;
    virtual void P_SetPhysicsMask(PhysicsID targetID, const FPhysicsMask& physicsMask) = 0;


    // === 활성화 제어 ===

    virtual void P_SetPhysicsActive(PhysicsID targetID, bool bActive) = 0;
    virtual bool P_IsPhysicsActive(PhysicsID targetID) const = 0;

    virtual void P_SetCollisionActive(PhysicsID targetID, bool bActive) = 0;
    virtual bool P_IsCollisionActive(PhysicsID targetID) const = 0;

};