#pragma once
#include "Math.h"

//물리시뮬레이션 내부에서만 사용하는 물리 상태 인터페이스, 즉각 데이터가 수정될 수 있음.
class IPhysicsStateInternal
{
public:
    virtual float GetMassInternal() const = 0;
    virtual Vector3 GetRotationalInertiaInternal() const = 0;
    virtual float GetRestitutionInternal() const = 0;
    virtual float GetFrictionStaticInternal() const = 0;
    virtual float GetFrictionKineticInternal() const = 0;

    virtual Vector3 GetVelocityInternal() const = 0;
    virtual Vector3 GetAngularVelocityInternal() const = 0;
    virtual Quaternion GetWorldRotationInternal() const = 0;
    virtual Vector3 GetWorldPositionInternal() const = 0;

    virtual void SetWorldPositionInternal(const Vector3& InPosition) = 0;

    virtual void ApplyForceInternal(const Vector3& Force) = 0;
    virtual void ApplyImpulseInternal(const Vector3& Impulse) = 0;
    virtual void ApplyForceInternal(const Vector3& Force, const Vector3& Location) = 0;
    virtual void ApplyImpulseInternal(const Vector3& Impulse, const Vector3& Location) = 0;

    virtual void SetVelocityInternal(const Vector3& InVelocity) = 0;
    virtual void AddVelocityInternal(const Vector3& InVelocityDelta) = 0;
    virtual void SetAngularVelocityInternal(const Vector3& InAngularVelocity) = 0;
    virtual void AddAngularVelocityInternal(const Vector3& InAngularVelocityDelta) = 0;
};