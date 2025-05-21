#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsStateInterface.h"
#include "PhysicsStateInternalInterface.h"
#include "PhysicsObjectInterface.h"
#include "DynamicCircularQueue.h"

class UGameObject;

enum class ERigidBodyType
{
	Dynamic,
	Static
};

class URigidBodyComponent : public USceneComponent ,
	public IPhysicsState, public IPhysicsObejct,  public IPhysicsStateInternal
{
public:
	// 회전관성 접근제어 토근
	class RotationalInertiaToken
	{
		friend class UCollisionComponentBase;
	private:
		RotationalInertiaToken() = default;
	};

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

	inline float GetMass() const override { return IsStatic() ? 1 / KINDA_SMALL : CachedState.Mass; }
	inline Vector3 GetRotationalInertia() const override { return CachedState.RotationalInertia; }
	inline float GetRestitution() const override { return CachedState.Restitution; }
	inline float GetFrictionKinetic() const override { return CachedState.FrictionKinetic; }
	inline float GetFrictionStatic() const override { return CachedState.FrictionStatic; }

	inline float GetSpeed() const { return CachedState.Velocity.Length(); }

#pragma endregion
#pragma region IPhyscisStateInternal
	//TODO : for test, simple redirection
	float P_GetMass() const override { return GetMass();}
	Vector3 P_GetRotationalInertia() const override { return GetRotationalInertia(); }

	float P_GetRestitution() const override { return GetRestitution(); }
    float P_GetFrictionStatic() const override { return GetFrictionStatic(); }
    float P_GetFrictionKinetic() const override { return GetFrictionKinetic(); }

    Vector3 P_GetVelocity() const override { return GetVelocity(); }
    Vector3 P_GetAngularVelocity() const override { return GetAngularVelocity(); }

    const FTransform& P_GetWorldTransform() const override { return GetWorldTransform(); }

    void P_SetWorldPosition(const Vector3& InPoisiton) override { SetWorldPosition(InPoisiton); }
    void P_SetWorldRotation(const Quaternion& InQuat) override { SetWorldRotation(InQuat); }
    void P_SetWorldScale(const Vector3& InScale) override { SetWorldScale(InScale); }

    void P_ApplyForce(const Vector3& Force) override { ApplyForce(Force); }
    void P_ApplyImpulse(const Vector3& Impulse) override { ApplyImpulse(Impulse); }

    void P_ApplyForce(const Vector3& Force, const Vector3& Location) override { ApplyForce(Force, Location); }
    void P_ApplyImpulse(const Vector3& Impulse, const Vector3& Location) override { ApplyImpulse(Impulse, Location); }

    void P_SetVelocity(const Vector3& InVelocity) override { SetVelocity(InVelocity); }
    void P_AddVelocity(const Vector3& InVelocityDelta) override { AddVelocity(InVelocityDelta); }

    void P_SetAngularVelocity(const Vector3& InAngularVelocity) override { SetAngularVelocity(InAngularVelocity); }
    void P_AddAngularVelocity(const Vector3& InAngularVelocityDelta) override { AddAngularVelocity(InAngularVelocityDelta); }
#pragma region
#pragma region IPhysicsObject
	// 현재 상태를 외부 상태로 저장
	void SynchronizeCachedStateFromSimulated()  override;
	// 외부 상태를 현재 상태로 저장
	void UpdateSimulatedStateFromCached() const override;
	bool IsDirtyPhysicsState() const override;
	bool IsActive() const override;
#pragma endregion

	// 물리 속성 설정
	void SetMass(float InMass);
	inline void SetMaxSpeed(float InSpeed) { MaxSpeed = InSpeed; }
	inline void SetMaxAngularSpeed(float InSpeed) { MaxAngularSpeed = InSpeed; }
	inline void SetGravityScale(float InScale) { GravityScale = InScale; }
	inline void SetFrictionKinetic(float InFriction) { CachedState.FrictionKinetic = InFriction; }
	inline void SetFrictionStatic(float InFriction) { CachedState.FrictionStatic = InFriction; }
	inline void SetRestitution(float InRestitution) { CachedState.Restitution = InRestitution; }

	inline void SetRigidType(ERigidBodyType&& InType) { CachedState.RigidType = InType; }

	//토큰소유자만 접근 가능
	void SetRotationalInertia(const Vector3& Value, const RotationalInertiaToken&) { CachedState.RotationalInertia = Value; }

	virtual const char* GetComponentClassName() const override { return "URigid"; }

public:
	// 시뮬레이션 플래그
	bool bGravity  =  false;

	bool IsStatic() const { return CachedState.RigidType == ERigidBodyType::Static; }

private:
	void RegisterPhysicsSystem() override;
	void UnRegisterPhysicsSystem() override;

	//속도에 따른 트랜스폼 변화
	void UpdateTransform(float DeltaTime);
	void ClampVelocities(Vector3& OutVelocity, Vector3& OutAngularVelocity);
	void ClampLinearVelocity(Vector3& OutVelocity);
	void ClampAngularVelocity(Vector3& OutAngularVelocity);
	void ApplyDrag(float DeltaTime) {}//todo
	Vector3 GetCenterOfMass() const;

	__forceinline bool IsSpeedRestricted() { return !(MaxSpeed < 0.0f); }
	__forceinline bool IsAngularSpeedRestricted() { return !(MaxAngularSpeed < 0.0f); }

private:

	struct FRigidPhysicsState
	{
		// 물리 상태 변수
		Vector3 Velocity = Vector3::Zero();
		Vector3 AngularVelocity = Vector3::Zero();
		Vector3 AccumulatedForce = Vector3::Zero();
		Vector3 AccumulatedTorque = Vector3::Zero();
		Vector3 AccumulatedInstantForce = Vector3::Zero();
		Vector3 AccumulatedInstantTorque = Vector3::Zero();

		// 물리 속성
		float Mass = 1.0f;
		Vector3 RotationalInertia = Vector3::One();
		float FrictionKinetic = 0.3f;
		float FrictionStatic = 0.5f;
		float Restitution = 0.5f;

		// 물리 객체 상태
		ERigidBodyType RigidType = ERigidBodyType::Dynamic;

		FTransform CachedWorldTransform = FTransform();
		FTransform CachedLocalTransform = FTransform();
	};

	mutable bool bStateDirty = false;
	FRigidPhysicsState CachedState;  //저장된 물리 상태값, 외부에 읽기전용으로 제공될것
	mutable FRigidPhysicsState SimulatedState;  //물리시스템 내부 연산용 물리 상태값. 

	float MaxSpeed = 400.0f;
	float MaxAngularSpeed = 6.0f * PI;

	float GravityScale = 9.81f;
	Vector3 GravityDirection = -Vector3::Up();

#pragma region PhysicsRequest Queue
	//private:

	//private:
	//	TCircularQueue
#pragma endregion
};