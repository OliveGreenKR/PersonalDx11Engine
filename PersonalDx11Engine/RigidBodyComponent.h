#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"

class UGameObject;

enum class ERigidBodyType
{
    Dynamic,
    Static
};

class URigidBodyComponent : public UActorComponent
{
public:
    URigidBodyComponent() = default;

    void Reset();
    virtual void Tick(const float DeltaTime) override;

    // 속도 기반 인터페이스
    void SetVelocity(const Vector3& InVelocity);
    void AddVelocity(const Vector3& InVelocityDelta);
    void SetAngularVelocity(const Vector3& InAngularVelocity);
    void AddAngularVelocity(const Vector3& InAngularVelocityDelta);

    // 힘 기반 인터페이스 (내부적으로 가속도로 변환)
    inline void ApplyForce(const Vector3& Force) { ApplyForce(Force, GetCenterOfMass()); }
    void ApplyForce(const Vector3& Force, const Vector3& Location);
    inline void ApplyImpulse(const Vector3& Impulse) { ApplyImpulse(Impulse, GetCenterOfMass()); }
    void ApplyImpulse(const Vector3& Impulse, const Vector3& Location);

    // Getters
    inline const Vector3& GetVelocity() const { return Velocity; }
    inline const Vector3& GetAngularVelocity() const { return AngularVelocity; }
    inline float GetSpeed() const { return Velocity.Length(); }
    inline float GetMass() const { return Mass; }
    inline float GetRotationalInertia() const { return RotationalInertia; }
    inline float GetRestitution() const { return Restitution; }
    inline float GetFrictionKinetic() const { return FrictionKinetic; }
    inline float GetFrictionStatic() const { return FrictionStatic; }

    const struct FTransform* GetTransform() const;

    // 물리 속성 설정
    void SetMass(float InMass);
    inline void SetMaxSpeed(float InSpeed) { MaxSpeed = InSpeed; }
    inline void SetMaxAngularSpeed(float InSpeed) { MaxAngularSpeed = InSpeed; }
    inline void SetGravityScale(float InScale) { GravityScale = InScale; }
    inline void SetFrictionKinetic(float InFriction) { FrictionKinetic = InFriction; }
    inline void SetFrictionStatic(float InFriction) { FrictionStatic = InFriction; }
    inline void SetRestitution(float InRestitution) { Restitution = InRestitution; }

    ////Owner설정
    //inline void SetOwner(std::shared_ptr<UGameObject>& InOwner) { Owner = InOwner; }
public:
    // 시뮬레이션 플래그
    bool bGravity  =  false;
    bool bIsSimulatedPhysics  = true;
    bool IsStatic() { return RigidType == ERigidBodyType::Static; }

private:
    void UpdateTransform(float DeltaTime);
    void ClampVelocities();
    void ApplyDrag(float DeltaTime) {}//todo
    Vector3 GetCenterOfMass() const;

private:
    // 물리 객체 상태
    ERigidBodyType RigidType = ERigidBodyType::Dynamic;

    // 물리 상태 변수
    Vector3 Velocity = Vector3::Zero;
    Vector3 AngularVelocity = Vector3::Zero;
    Vector3 AccumulatedForce = Vector3::Zero;
    Vector3 AccumulatedTorque = Vector3::Zero;
    Vector3 AccumulatedInstantForce = Vector3::Zero;
    Vector3 AccumulatedInstantTorque = Vector3::Zero;

    // 물리 속성
    float Mass = 1.0f;
    float RotationalInertia = 1.0f;
    float MaxSpeed = 5.0f;
    float MaxAngularSpeed = 6.0f * PI;
    float FrictionKinetic = 0.3f;
    float FrictionStatic = 0.5f;
    float Restitution = 0.5f;
    float LinearDrag = 0.01f;
    float AngularDrag = 0.01f;

    float GravityScale = 9.81f;
    Vector3 GravityDirection = -Vector3::Up;


    //std::weak_ptr<UGameObject> Owner;


};