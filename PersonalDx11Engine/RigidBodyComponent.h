#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsStateInterface.h"
#include "PhysicsDefine.h"
#include "PhysicsObjectInterface.h"
#include "PhysicsJob.h"

class UGameObject;

enum class ERigidBodyType
{
	Dynamic,
	Static
};

class URigidBodyComponent : public USceneComponent ,
	public IPhysicsState, public IPhysicsObject
{
public:
	URigidBodyComponent();
	~URigidBodyComponent();

	virtual void PostInitialized() override;

	virtual void Activate() override;
	virtual void DeActivate() override;

	void Reset();

	virtual void Tick(const float DeltaTime) override;

	//IPhysicsObject
	virtual void TickPhysics(const float DeltaTime) override;

	//SceneComp override
	void SetWorldTransform(const FTransform& InWorldTransform) override;

#pragma region IPhysicsState
public:
	// 속도 기반 인터페이스
	void SetVelocity(const Vector3& InVelocity) override;
	void AddVelocity(const Vector3& InVelocityDelta) override;
	void SetAngularVelocity(const Vector3& InAngularVelocity) override;
	void AddAngularVelocity(const Vector3& InAngularVelocityDelta) override;

	// 힘 기반 인터페이스 (내부적으로 가속도로 변환)
 	inline void ApplyForce(const Vector3& Force) override  { ApplyForce(Force, GetCenterOfMass()); }
	void ApplyForce(const Vector3& Force, const Vector3& Location) override ;
	inline void ApplyImpulse(const Vector3& Impulse) { ApplyImpulse(Impulse, GetCenterOfMass()); }
	void ApplyImpulse(const Vector3& Impulse, const Vector3& Location) override;

	// Getters
	inline Vector3 GetVelocity() const override { return CachedState.Velocity;}
	inline Vector3 GetAngularVelocity() const override { return CachedState.AngularVelocity; }

	inline float GetMass() const override
	{
		float invMass = GetInvMass();
		return invMass > 0.0f ? 1.0f / invMass : KINDA_LARGE;
	}
	inline float GetInvMass() const override { return IsStatic() ? 0.0f : CachedState.InvMass; }

	inline Vector3 GetRotationalInertia() const override 
	{
		Vector3 Result;
		Vector3 InvRotationalInertia = CachedState.InvRotationalInertia;;
		Result.x = (abs(InvRotationalInertia.x) < KINDA_SMALL) ?
			KINDA_LARGE : (1.0f / InvRotationalInertia.x);
		Result.y = (abs(InvRotationalInertia.y) < KINDA_SMALL) ?
			KINDA_LARGE : (1.0f / InvRotationalInertia.y);
		Result.z = (abs(InvRotationalInertia.z) < KINDA_SMALL) ?
			KINDA_LARGE : (1.0f / InvRotationalInertia.z);
		return Result;
	}
	inline Vector3 GetInvRotationalInertia() const override { return CachedState.InvRotationalInertia; }

	inline float GetRestitution() const override { return CachedState.Restitution; }
	inline float GetFrictionKinetic() const override { return CachedState.FrictionKinetic; }
	inline float GetFrictionStatic() const override { return CachedState.FrictionStatic; }

	inline float GetSpeed() const { return CachedState.Velocity.Length(); }

#pragma endregion

#pragma region IPhysicsObject
public:
	// 현재 상태를 외부 상태로 저장
	void SynchronizeCachedStateFromSimulated()  override;
	bool IsActive() const override;
	PhysicsID GetPhysicsID() const override { return PhysicsID; }
	void SetPhysicsID(uint32_t InID) override { PhysicsID = InID; }
#pragma endregion
public:
	// 물리 속성 설정
	void SetMass(float InMass);
	void SetFrictionKinetic(float InFriction);
	void SetFrictionStatic(float InFriction);
	void SetRestitution(float InRestitution);
	void SetRigidType(ERigidBodyType&& InType);
	void SetInvRotationalInertia(const Vector3& Value);

	virtual const char* GetComponentClassName() const override { return "URigid"; }

	inline void SetMaxSpeed(float InSpeed);
	inline void SetMaxAngularSpeed(float InSpeed);
	inline void SetGravityScale(float InScale);
public:
	// 시뮬레이션 플래그
	void SetGravity(const bool InBool);
	bool IsGravity() const { return CachedState.PhysicsMasks.HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED); }
	bool IsStatic() const { return CachedState.PhysicsType == EPhysicsType::Static; }

private:
	void RegisterPhysicsSystem() override;
	void UnRegisterPhysicsSystem() override;

	Vector3 GetCenterOfMass() const;

private:
	PhysicsID PhysicsID = 0;

	mutable bool bStateDirty = false;
	FPhysicsState CachedState;  //저장된 물리 상태값, 외부에 읽기전용으로 제공될것
};