#pragma once
#include <memory>
#include "Math.h"

class UGameObject;

// 물리 속성을 관리하는 RigidBody 컴포넌트
class URigidBodyComponent 
{
public:
    URigidBodyComponent() = default;
    URigidBodyComponent(const std::shared_ptr<class UGameObject>& InOwner) : Owner(InOwner)
    {};

    void Reset();
    void SetMass(float InMass) { Mass = Math::Max(InMass, KINDA_SMALL); }
    void SetMaxSpeed(float InMaxSpeed) { MaxSpeed = InMaxSpeed; }
    void SetFrictionCoefficient(float InFriction) { FrictionCoefficient = InFriction; }
    void SetGravity(const Vector3& InGravity) { Gravity = InGravity; }
    void EnableGravity(bool bEnable) { bUseGravity = bEnable; }
    void EnablePhysics(bool bEnable) { bIsSimulatedPhysics = bEnable; }

    const Vector3& GetVelocity() const { return Velocity; }
    float GetSpeed() const { return Velocity.Length(); }
    float GetMass() const { return Mass; }

    void Tick(const float DeltaTime);
    void ApplyForce(const Vector3& Force);
    void ApplyImpulse(const Vector3& InINpulse);

    void SetOwner(std::shared_ptr<class UGameObject>& InOwner) { Owner = InOwner; }

private:
    bool bUseGravity = false;
    bool bIsSimulatedPhysics = true;

    //physics
    float Mass = 1.0f;
    float MaxSpeed = 50.0f;
    float FrictionCoefficient = 0.5f;
    float GravityScale = 9.8f;
    Vector3 Gravity = -Vector3::Up;

    //state
    Vector3 Velocity = Vector3::Zero;
    Vector3 AccumulatedForce = Vector3::Zero;

    std::weak_ptr<class UGameObject> Owner;
};
