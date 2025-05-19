#pragma once
#include "Math.h"

class IPhysicsState
{
public:
    virtual void SynchronizeState() = 0;
    virtual void CaptureState() const = 0;
    virtual bool IsActive() const = 0;
    virtual bool IsDirty() const = 0;
    virtual bool IsStatic() const = 0;

    virtual void SetWorldPosition(const Vector3& InPosition) = 0;

	virtual float GetMass() const = 0;
    virtual Vector3 GetRotationalInertia() const = 0;
    virtual Vector3 GetWorldPosition() const = 0;
    virtual Vector3 GetVelocity() const = 0;
    virtual Vector3 GetAngularVelocity() const = 0;
    virtual Quaternion GetWorldRotation() const = 0;

    virtual float GetRestitution() const = 0;
    virtual float GetFrictionStatic() const = 0;
    virtual float GetFrictionKinetic() const = 0;

    virtual void ApplyForce(const Vector3& Force) = 0;
    virtual void ApplyImpulse(const Vector3& Impulse) = 0;
    virtual void ApplyForce(const Vector3& Force, const Vector3& Location) = 0;
    virtual void ApplyImpulse(const Vector3& Impulse, const Vector3& Location) = 0;

    virtual void SetVelocity(const Vector3& InVelocity) = 0;
    virtual void AddVelocity(const Vector3& InVelocityDelta) = 0;
    virtual void SetAngularVelocity(const Vector3& InAngularVelocity) = 0;
    virtual void AddAngularVelocity(const Vector3& InAngularVelocityDelta) = 0;
};