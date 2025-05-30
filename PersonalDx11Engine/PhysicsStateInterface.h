#pragma once
#include "Math.h"

//물리시뮬레이션 외부, 메인 로직에서 접근하는 외부 물리 상태 인터페이스
class IPhysicsState
{
public:
	virtual float GetMass() const = 0;
    virtual float GetInvMass() const = 0;
    virtual Vector3 GetRotationalInertia() const = 0;
    virtual Vector3 GetInvRotationalInertia() const = 0;
    virtual float GetRestitution() const = 0;
    virtual float GetFrictionStatic() const = 0;
    virtual float GetFrictionKinetic() const = 0;

    virtual Vector3 GetVelocity() const = 0;
    virtual Vector3 GetAngularVelocity() const = 0;

    virtual void ApplyForce(const Vector3& Force) = 0;
    virtual void ApplyImpulse(const Vector3& Impulse) = 0;
    virtual void ApplyForce(const Vector3& Force, const Vector3& Location) = 0;
    virtual void ApplyImpulse(const Vector3& Impulse, const Vector3& Location) = 0;

    virtual void SetVelocity(const Vector3& InVelocity) = 0;
    virtual void AddVelocity(const Vector3& InVelocityDelta) = 0;
    virtual void SetAngularVelocity(const Vector3& InAngularVelocity) = 0;
    virtual void AddAngularVelocity(const Vector3& InAngularVelocityDelta) = 0;
};