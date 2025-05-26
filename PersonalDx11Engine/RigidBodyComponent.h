#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsStateInterface.h"
#include "PhysicsStateInternalInterface.h"
#include "PhysicsObjectInterface.h"
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

	friend class FPhysicsJob;

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
public:
	//TODO : for test, simple redirection
	float P_GetMass() const override { return SimulatedState.Mass;}
	Vector3 P_GetRotationalInertia() const override { return SimulatedState.RotationalInertia; }

    float P_GetRestitution() const override { return SimulatedState.Restitution; }
    float P_GetFrictionStatic() const override { return SimulatedState.FrictionStatic; }
    float P_GetFrictionKinetic() const override { return SimulatedState.FrictionKinetic; }

    Vector3 P_GetVelocity() const override { return SimulatedState.Velocity; }
    Vector3 P_GetAngularVelocity() const override { return SimulatedState.AngularVelocity; }

    const FTransform& P_GetWorldTransform() const override { return SimulatedState.WorldTransform; }
	Vector3 P_GetCenterOfMass() const { return SimulatedState.WorldTransform.Position; }

	void P_SetWorldPosition(const Vector3& InPoisiton) override;
	void P_SetWorldRotation(const Quaternion& InQuat) override;
	void P_SetWorldScale(const Vector3& InScale) override;

	void P_ApplyForce(const Vector3& Force) override { P_ApplyForce(Force, P_GetCenterOfMass()); }
	void P_ApplyImpulse(const Vector3& Impulse) override{ P_ApplyImpulse(Impulse, P_GetCenterOfMass()); }

	void P_ApplyForce(const Vector3& Force, const Vector3& Location) override;
    void P_ApplyImpulse(const Vector3& Impulse, const Vector3& Location) override;

    void P_SetVelocity(const Vector3& InVelocity) override;
    void P_AddVelocity(const Vector3& InVelocityDelta) override;

    void P_SetAngularVelocity(const Vector3& InAngularVelocity) override;
    void P_AddAngularVelocity(const Vector3& InAngularVelocityDelta) override;
#pragma endregion
#pragma region IPhysicsObject
public:
	// 현재 상태를 외부 상태로 저장
	void SynchronizeCachedStateFromSimulated()  override;
	// 외부 상태를 현재 상태로 저장
	void UpdateSimulatedStateFromCached() override;
	bool IsDirtyPhysicsState() const override;
	bool IsActive() const override;
	bool IsSleep() const override;
#pragma endregion
public:
	// 물리 속성 설정
	void SetMass(float InMass);
	void SetFrictionKinetic(float InFriction);
	void SetFrictionStatic(float InFriction);
	void SetRestitution(float InRestitution);
	void SetRigidType(ERigidBodyType&& InType);

	//토큰소유자만 접근 가능
	void SetRotationalInertia(const Vector3& Value, const RotationalInertiaToken&);

	virtual const char* GetComponentClassName() const override { return "URigid"; }

	inline void SetMaxSpeed(float InSpeed) { MaxSpeed = InSpeed; }
	inline void SetMaxAngularSpeed(float InSpeed) { MaxAngularSpeed = InSpeed; }
	inline void SetGravityScale(float InScale) { GravityScale = InScale; }
public:
	// 시뮬레이션 플래그
	void SetGravity(const bool InBool);
	bool IsGravity() const { return bGravity; }
	bool IsStatic() const { return CachedState.RigidType == ERigidBodyType::Static; }

private:
	void RegisterPhysicsSystem() override;
	void UnRegisterPhysicsSystem() override;

	//속도에 따른 트랜스폼 변화
	void P_UpdateTransformByVelocity(float DeltaTime);
	void ClampVelocities(Vector3& OutVelocity, Vector3& OutAngularVelocity);
	void ClampLinearVelocity(Vector3& OutVelocity);
	void ClampAngularVelocity(Vector3& OutAngularVelocity);
	void ApplyDrag(float DeltaTime) {}//todo
	Vector3 GetCenterOfMass() const;

	__forceinline bool IsSpeedRestricted() { return !(MaxSpeed < 0.0f); }
	__forceinline bool IsAngularSpeedRestricted() { return !(MaxAngularSpeed < 0.0f); }

	bool ShouldSleep() const;

	//물리 상태값 적분 안정화를 위한 헬퍼함수
	static bool IsValidForce(const Vector3& InForce);
	static bool IsValidTorque(const Vector3& InTorque);
	static bool IsValidVelocity(const Vector3& InVelocity);
	static bool IsValidAngularVelocity(const Vector3& InAngularVelocity);
	static bool IsValidAcceleration(const Vector3& InAccel);
	static bool IsValidAngularAcceleration(const Vector3& InAngularAccel);
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

		FTransform WorldTransform;

		void Reset()
		{
			Velocity = Vector3::Zero();
			AngularVelocity = Vector3::Zero();
			AccumulatedForce = Vector3::Zero();
			AccumulatedTorque = Vector3::Zero();
			AccumulatedInstantForce = Vector3::Zero();
			AccumulatedInstantTorque = Vector3::Zero();
		}
	};

	mutable bool bStateDirty = false;
	FRigidPhysicsState CachedState;  //저장된 물리 상태값, 외부에 읽기전용으로 제공될것
	mutable FRigidPhysicsState SimulatedState;  //물리시스템 내부 연산용 물리 상태값. 

	float MaxSpeed = 400.0f;
	float MaxAngularSpeed = 6.0f * PI;

	bool bGravity = false;
	float GravityScale = 9.81f;
	Vector3 GravityDirection = -Vector3::Up();

	bool bSleep = false;
	

	private:

};