#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsStateInterface.h"
#include "PhysicsObjectInterface.h"

class UGameObject;

enum class ERigidBodyType
{
	Dynamic,
	Static
};

class URigidBodyComponent : public USceneComponent ,
	public IPhysicsState, public IPhysicsObejct
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

	inline float GetMass() const override { return IsStatic() ? 1 / KINDA_SMALL : Mass; }
	inline Vector3 GetRotationalInertia() const override { return RotationalInertia; }
	inline float GetRestitution() const override { return Restitution; }
	inline float GetFrictionKinetic() const override { return FrictionKinetic; }
	inline float GetFrictionStatic() const override { return FrictionStatic; }

	inline float GetSpeed() const { return CachedState.Velocity.Length(); }

	// Inherited via IPhysicsState
	Vector3 GetWorldPosition() const override { return USceneComponent::GetWorldPosition(); }
	Quaternion GetWorldRotation() const override { return USceneComponent::GetWorldRotation(); }

	inline void SetWorldPosition(const Vector3& InWorldPosition) override {
		USceneComponent::SetWorldPosition(InWorldPosition);
	}
#pragma endregion
#pragma region IPhysicsObject
	// Inherited via IPhysicsState
	//현재 상태를 외부상태로 변경(외부 상태 변화 반영)
	void SynchronizeState()  override;
	//외부 상태를 현재상태로 변경(현재 상태 변화 반영)
	void CaptureState() const override;
	bool IsDirtyPhysicsState() const override;
	bool IsActive() const override;
#pragma endregion

	// 물리 속성 설정
	void SetMass(float InMass);
	inline void SetMaxSpeed(float InSpeed) { MaxSpeed = InSpeed; }
	inline void SetMaxAngularSpeed(float InSpeed) { MaxAngularSpeed = InSpeed; }
	inline void SetGravityScale(float InScale) { GravityScale = InScale; }
	inline void SetFrictionKinetic(float InFriction) { FrictionKinetic = InFriction; }
	inline void SetFrictionStatic(float InFriction) { FrictionStatic = InFriction; }
	inline void SetRestitution(float InRestitution) { Restitution = InRestitution; }

	inline void SetRigidType(ERigidBodyType&& InType) { RigidType = InType; }

	//토큰소유자만 접근 가능
	void SetRotationalInertia(const Vector3& Value, const RotationalInertiaToken&) { RotationalInertia = Value; }

	virtual const char* GetComponentClassName() const override { return "URigid"; }

public:
	// 시뮬레이션 플래그
	bool bGravity  =  false;

	bool IsStatic() const { return RigidType == ERigidBodyType::Static; }

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
	};

	mutable bool bStateDirty = false;
	//저장된 물리 상태값, 외부에 제공되고 수정될 수 있음.
	mutable FRigidPhysicsState CachedState;
	//실제 물리 상태값. Rigidbody는 해당 상태값에 따른 로직을 수행함.
	FRigidPhysicsState CurrentState;


	// 물리 속성
	float Mass = 1.0f;
	Vector3 RotationalInertia = Vector3::One();
	float FrictionKinetic = 0.3f;
	float FrictionStatic = 0.5f;
	float Restitution = 0.5f;
	float LinearDrag = 0.01f;
	float AngularDrag = 0.01f;

	// 물리 객체 상태
	ERigidBodyType RigidType = ERigidBodyType::Dynamic;

	float MaxSpeed = 400.0f;
	float MaxAngularSpeed = 6.0f * PI;

	float GravityScale = 9.81f;
	Vector3 GravityDirection = -Vector3::Up();
};