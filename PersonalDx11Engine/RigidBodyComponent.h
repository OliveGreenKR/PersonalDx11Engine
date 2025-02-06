#pragma once
#include <memory>
#include "Math.h"

class UGameObject;


// 물리 속성을 관리하는 RigidBody 컴포넌트
//Force를 통한 단일 외부 인터페이스를 이용해 객체의 상태를 다룰 에정.
class URigidBodyComponent 
{
public:
    URigidBodyComponent() = default;
    URigidBodyComponent(const std::shared_ptr<class UGameObject>& InOwner) : Owner(InOwner)
    {};

    void Reset();
    void SetOwner(const std::shared_ptr<class UGameObject>& InOwner) { Owner = InOwner; }
    void SetMass(float InMass) { Mass = Math::Max(InMass, KINDA_SMALL); }

    void SetMaxSpeed(float InMaxSpeed) { MaxSpeed = InMaxSpeed; }
    void SetMaxAngularSpeed(float InMaxSpeed) { MaxAngularSpeed = InMaxSpeed; }
    void SetFrictionKinetic(float InFriction) { FrictionKinetic = InFriction; }
    void SetFrictionStatic(float InFriction) { FrictionStatic = InFriction; }
    void SetGravityDirection(const Vector3& InGravity) { Gravity = InGravity; }

    const Vector3& GetVelocity() const { return Velocity; }
    const Vector3& GetAngularVelocity() const { return AngularVelocity; }
    float GetSpeed() const { return Velocity.Length(); }
    float GetMass() const { return Mass; }
    float GetRotationalInertia() const { return RotationalInertia; }

    void Tick(const float DeltaTime);

    inline void ApplyForce(const Vector3& Force) { ApplyForce(Force, GetCenterOfMass());}
    void ApplyForce(const Vector3& Force, const Vector3& ApplyPosition);
    inline void ApplyImpulse(const Vector3& InImpulse) { ApplyImpulse(InImpulse, GetCenterOfMass()); }
    void ApplyImpulse(const Vector3& InImpulse, const Vector3& ApplyPosition);

    void SetOwner(std::shared_ptr<class UGameObject>& InOwner) { Owner = InOwner; }

private:
    const Vector3& GetCenterOfMass();
    void ApplyLinearForce(const Vector3& Force);
    void ApplyTorque(const Vector3& Torque);
    void ClampVelocities();

public:
    bool bGravity = false;
    bool bIsSimulatedPhysics = true;

private:
    //physics
    float Mass = 1.0f;
    float MaxSpeed = 50.0f;
    float MaxAngularSpeed = 6.0f * PI;
    float FrictionKinetic = 0.3f;
    float FrictionStatic = 0.5f;
    float GravityScale = 9.8f;
    Vector3 Gravity = -Vector3::Up;
    float RotationalInertia = 1.0f;


    //state
    Vector3 Velocity = Vector3::Zero;
    Vector3 AngularVelocity = Vector3::Zero;
    Vector3 AccumulatedForce = Vector3::Zero;
    Vector3 AccumulatedTorque = Vector3::Zero;

    std::weak_ptr<class UGameObject> Owner;
};
