#pragma once
#include <memory>
#include "Math.h"
#include "ObjectComponentInterface.h"

using namespace DirectX;

// 물리 속성을 관리하는 RigidBody 컴포넌트
class URigidBodyComponent : public IObejctCompoenent
{
public:
    URigidBodyComponent(std::shared_ptr<UGameObject>& InOwner) : Owner(InOwner) {
        bIsTicked = true;
    }

    void SetMass(float InMass) { Mass = InMass; }
    void SetVelocity(const Vector3& InVelocity) { Velocity = InVelocity; }
    void SetAcceleration(const Vector3& InAcceleration) { Acceleration = InAcceleration; }

    void Tick(float DeltaTime);
    void ApplyForce(const Vector3& Force);

protected:
    virtual void Tick(float DeltaTime) override;

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
    Vector3 Velocity;
    Vector3 AccumulatedForce;
};
