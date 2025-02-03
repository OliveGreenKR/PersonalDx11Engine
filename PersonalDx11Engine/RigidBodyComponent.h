#pragma once
#include <memory>
#include "Math.h"

class UGameObject;
 
// �浹 ���¸� �����ϴ� ������
enum class ECollisionShape
{
    Box,
    Sphere,
};

// ���� �Ӽ��� �����ϴ� RigidBody ������Ʈ
//Force�� ���� ���� �ܺ� �������̽��� �̿��� ��ü�� ���¸� �ٷ� ����.
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
    void SetFrictionKinetic(float InFriction) { FrictionKinetic = InFriction; }
    void SetFrictionStatic(float InFriction) { FrictionStatic = InFriction; }
    void SetGravityDirection(const Vector3& InGravity) { Gravity = InGravity; }
    void SetGravity(bool bEnable) { bGravity = bEnable; }
    void SetPhysics(bool bEnable) { bIsSimulatedPhysics = bEnable; }

    const Vector3& GetVelocity() const { return Velocity; }
    float GetSpeed() const { return Velocity.Length(); }
    float GetMass() const { return Mass; }

    void Tick(const float DeltaTime);
    void ApplyForce(const Vector3& Force);
    void ApplyImpulse(const Vector3& InINpulse);

    void SetOwner(std::shared_ptr<class UGameObject>& InOwner) { Owner = InOwner; }

private:
    bool bGravity = false;
    bool bIsSimulatedPhysics = true;

    //physics
    float Mass = 1.0f;
    float MaxSpeed = 50.0f;
    float FrictionKinetic = 0.5f;
    float FrictionStatic = 0.8f;
    float GravityScale = 9.8f;
    Vector3 Gravity = -Vector3::Up;

    //state
    Vector3 Velocity = Vector3::Zero;
    Vector3 AccumulatedForce = Vector3::Zero;

    std::weak_ptr<class UGameObject> Owner;
};
