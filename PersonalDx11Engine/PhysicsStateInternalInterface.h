#pragma once
#include "Math.h"
#include "Transform.h"

//물리시뮬레이션 내부에서만 사용하는 물리 상태 인터페이스, 즉각 데이터가 수정될 수 있음.
class IPhysicsStateInternal
{
public:
    virtual float P_GetMass() const = 0;
    virtual Vector3 P_GetRotationalInertia() const = 0;
    virtual float P_GetRestitution() const = 0;
    virtual float P_GetFrictionStatic() const = 0;
    virtual float P_GetFrictionKinetic() const = 0;

    virtual Vector3 P_GetVelocity() const = 0;
    virtual Vector3 P_GetAngularVelocity() const = 0;

    virtual const FTransform& P_GetWorldTransform() const = 0;
    virtual void P_SetWorldTransform(const FTransform& InTransform) = 0;
    virtual void P_SetWorldPosition(const Vector3& InPoisiton) = 0;
    virtual void P_SetWorldRotation(const Quaternion& InQuat) = 0;
    virtual void P_SetWorldScale(const Vector3& InScale) = 0;

    virtual void P_ApplyForce(const Vector3& Force) = 0;
    virtual void P_ApplyImpulse(const Vector3& Impulse) = 0;
    virtual void P_ApplyForce(const Vector3& Force, const Vector3& Location) = 0;
    virtual void P_ApplyImpulse(const Vector3& Impulse, const Vector3& Location) = 0;

    virtual void P_SetVelocity(const Vector3& InVelocity) = 0;
    virtual void P_AddVelocity(const Vector3& InVelocityDelta) = 0;
    virtual void P_SetAngularVelocity(const Vector3& InAngularVelocity) = 0;
    virtual void P_AddAngularVelocity(const Vector3& InAngularVelocityDelta) = 0;
};