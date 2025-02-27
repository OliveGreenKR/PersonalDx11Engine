#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "PrimitiveComponent.h"

class UGameObject;

enum class ERigidBodyType
{
	Dynamic,
	Static
};

class URigidBodyComponent : public UPrimitiveComponent
{
public:
	// ȸ������ �������� ���
	class RotationalInertiaToken
	{
		friend class UCollisionComponent;
	private:
		RotationalInertiaToken() = default;
	};

public:
	URigidBodyComponent();

	void Reset();
	virtual void Tick(const float DeltaTime) override;

	virtual const FTransform* GetTransform() const override;
	virtual FTransform* GetTransform() override;

	// �ӵ� ��� �������̽�
	void SetVelocity(const Vector3& InVelocity);
	void AddVelocity(const Vector3& InVelocityDelta);
	void SetAngularVelocity(const Vector3& InAngularVelocity);
	void AddAngularVelocity(const Vector3& InAngularVelocityDelta);

	// �� ��� �������̽� (���������� ���ӵ��� ��ȯ)
	inline void ApplyForce(const Vector3& Force) { ApplyForce(Force, GetCenterOfMass()); }
	void ApplyForce(const Vector3& Force, const Vector3& Location);
	inline void ApplyImpulse(const Vector3& Impulse) { ApplyImpulse(Impulse, GetCenterOfMass()); }
	void ApplyImpulse(const Vector3& Impulse, const Vector3& Location);

	// Getters
	inline const Vector3& GetVelocity() const { return Velocity; }
	inline const Vector3& GetAngularVelocity() const { return AngularVelocity; }
	inline float GetSpeed() const { return Velocity.Length(); }
	inline float GetMass() const { return IsStatic() ? 1 / KINDA_SMALL : Mass; }
	inline Vector3 GetRotationalInertia() const { return RotationalInertia; }
	inline float GetRestitution() const { return Restitution; }
	inline float GetFrictionKinetic() const { return FrictionKinetic; }
	inline float GetFrictionStatic() const { return FrictionStatic; }

	// ���� �Ӽ� ����
	void SetMass(float InMass);
	inline void SetMaxSpeed(float InSpeed) { MaxSpeed = InSpeed; }
	inline void SetMaxAngularSpeed(float InSpeed) { MaxAngularSpeed = InSpeed; }
	inline void SetGravityScale(float InScale) { GravityScale = InScale; }
	inline void SetFrictionKinetic(float InFriction) { FrictionKinetic = InFriction; }
	inline void SetFrictionStatic(float InFriction) { FrictionStatic = InFriction; }
	inline void SetRestitution(float InRestitution) { Restitution = InRestitution; }

	inline void SetRigidType(ERigidBodyType&& InType) { RigidType = InType; }

	//��ū�����ڸ� ���� ����
	void SetRotationalInertia(const Vector3& Value, const RotationalInertiaToken&) { RotationalInertia = Value; } 

public:
	// �ùķ��̼� �÷���
	bool bGravity  =  false;
	bool bSyncWithOwner = true;

	bool IsStatic() const { return RigidType == ERigidBodyType::Static; }

private:
	void UpdateTransform(float DeltaTime);
	void ClampVelocities();
	void ApplyDrag(float DeltaTime) {}//todo
	Vector3 GetCenterOfMass() const;

	__forceinline bool IsSpeedRestricted() { return !(MaxSpeed < 0.0f); }
	__forceinline bool IsAngularSpeedRestricted() { return !(MaxAngularSpeed < 0.0f); }

private:
	// ���� ��ü ����
	ERigidBodyType RigidType = ERigidBodyType::Dynamic;

	// ���� ���� ����
	Vector3 Velocity = Vector3::Zero;
	Vector3 AngularVelocity = Vector3::Zero;
	Vector3 AccumulatedForce = Vector3::Zero;
	Vector3 AccumulatedTorque = Vector3::Zero;
	Vector3 AccumulatedInstantForce = Vector3::Zero;
	Vector3 AccumulatedInstantTorque = Vector3::Zero;

	// ���� �Ӽ�
	float Mass = 1.0f;
	Vector3 RotationalInertia = Vector3::One;
	float MaxSpeed = 4.0f;
	float MaxAngularSpeed = 6.0f * PI;
	float FrictionKinetic = 0.3f;
	float FrictionStatic = 0.5f;
	float Restitution = 0.5f;
	float LinearDrag = 0.01f;
	float AngularDrag = 0.01f;

	float GravityScale = 9.81f;
	Vector3 GravityDirection = -Vector3::Up;
};