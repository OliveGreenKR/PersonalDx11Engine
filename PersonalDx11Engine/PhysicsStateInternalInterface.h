#pragma once
#include "Math.h"
#include "Transform.h"
#include "PhysicsDefine.h"

/// <summary>
/// 물리 시뮬레이션 내부에서 사용하는 물리 상태 인터페이스 (SoA 기반)
/// 
/// 설계 변경 사항:
/// - 기존: 단일 객체 조작 (P_SetVelocity(const Vector3&))
/// - 신규: SoAID 기반 중앙 관리 (P_SetVelocity(SoAID, const Vector3&))
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

    // === 물리 속성 접근자 (SoAID 기반) ===

    virtual float P_GetMass(SoAID targetID) const = 0;
    virtual float P_GetInvMass(SoAID targetID) const = 0;
    virtual Vector3 P_GetRotationalInertia(SoAID targetID) const = 0;
    virtual Vector3 P_GetInvRotationalInertia(SoAID targetID) const = 0;
    virtual float P_GetRestitution(SoAID targetID) const = 0;
    virtual float P_GetFrictionStatic(SoAID targetID) const = 0;
    virtual float P_GetFrictionKinetic(SoAID targetID) const = 0;
    virtual float P_GetGravityScale(SoAID targetID) const = 0;
    virtual float P_GetMaxSpeed(SoAID targetID) const = 0;
    virtual float P_GetMaxAngularSpeed(SoAID targetID) const = 0;

    // === 운동 상태 접근자 (SoAID 기반) ===

    virtual Vector3 P_GetVelocity(SoAID targetID) const = 0;
    virtual Vector3 P_GetAngularVelocity(SoAID targetID) const = 0;
    virtual Vector3 P_GetAccumulatedForce(SoAID targetID) const = 0;
    virtual Vector3 P_GetAccumulatedTorque(SoAID targetID) const = 0;

    // === 트랜스폼 접근자 (SoAID 기반) ===

    virtual FTransform P_GetWorldTransform(SoAID targetID) const = 0;
    virtual Vector3 P_GetWorldPosition(SoAID targetID) const = 0;
    virtual Quaternion P_GetWorldRotation(SoAID targetID) const = 0;
    virtual Vector3 P_GetWorldScale(SoAID targetID) const = 0;

    // === 상태 타입 및 마스크 접근자 ===

    virtual EPhysicsType P_GetPhysicsType(SoAID targetID) const = 0;
    virtual FPhysicsMask P_GetPhysicsMask(SoAID targetID) const = 0;

    // === 운동 상태 설정자 (SoAID 기반) ===

    virtual void P_SetVelocity(SoAID targetID, const Vector3& velocity) = 0;
    virtual void P_AddVelocity(SoAID targetID, const Vector3& deltaVelocity) = 0;
    virtual void P_SetAngularVelocity(SoAID targetID, const Vector3& angularVelocity) = 0;
    virtual void P_AddAngularVelocity(SoAID targetID, const Vector3& deltaAngularVelocity) = 0;

    // === 트랜스폼 설정자 (SoAID 기반) ===

    virtual void P_SetWorldPosition(SoAID targetID, const Vector3& position) = 0;
    virtual void P_SetWorldRotation(SoAID targetID, const Quaternion& rotation) = 0;
    virtual void P_SetWorldScale(SoAID targetID, const Vector3& scale) = 0;

    /// <summary>
    /// Transform 전체 설정 - 개별 설정 메서드들을 조합
    /// </summary>
    void P_SetWorldTransform(SoAID targetID, const FTransform& transform)
    {
        P_SetWorldPosition(targetID, transform.Position);
        P_SetWorldRotation(targetID, transform.Rotation);
        P_SetWorldScale(targetID, transform.Scale);
    }

    // === 힘/충격 적용 (SoAID 기반) ===

    virtual void P_ApplyForce(SoAID targetID, const Vector3& force) = 0;
    virtual void P_ApplyForce(SoAID targetID, const Vector3& force, const Vector3& location) = 0;
    virtual void P_ApplyImpulse(SoAID targetID, const Vector3& impulse) = 0;
    virtual void P_ApplyImpulse(SoAID targetID, const Vector3& impulse, const Vector3& location) = 0;
    virtual void P_ApplyTorque(SoAID targetID, const Vector3& torque) = 0;

    // === 물리 속성 설정자 (SoAID 기반) ===

    virtual void P_SetMass(SoAID targetID, float mass) = 0;
    virtual void P_SetInvMass(SoAID targetID, float invMass) = 0;
    virtual void P_SetRotationalInertia(SoAID targetID, const Vector3& rotationalInertia) = 0;
    virtual void P_SetInvRotationalInertia(SoAID targetID, const Vector3& invRotationalInertia) = 0;
    virtual void P_SetRestitution(SoAID targetID, float restitution) = 0;
    virtual void P_SetFrictionStatic(SoAID targetID, float frictionStatic) = 0;
    virtual void P_SetFrictionKinetic(SoAID targetID, float frictionKinetic) = 0;
    virtual void P_SetGravityScale(SoAID targetID, float gravityScale) = 0;
    virtual void P_SetMaxSpeed(SoAID targetID, float maxSpeed) = 0;
    virtual void P_SetMaxAngularSpeed(SoAID targetID, float maxAngularSpeed) = 0;

    // === 상태 타입 및 마스크 설정자 ===

    virtual void P_SetPhysicsType(SoAID targetID, EPhysicsType physicsType) = 0;
    virtual void P_SetPhysicsMask(SoAID targetID, const FPhysicsMask& physicsMask) = 0;


    // === 활성화 제어 ===

    virtual void P_SetPhysicsActive(SoAID targetID, bool bActive) = 0;
    virtual bool P_IsPhysicsActive(SoAID targetID) const = 0;

    virtual void P_SetCollisionActive(SoAID targetID, bool bActive) = 0;
    virtual bool P_IsCollisionActive(SoAID targetID) const = 0;

};