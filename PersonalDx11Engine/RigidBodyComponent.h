#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsStateInterface.h"
#include "PhysicsDefine.h"
#include "PhysicsObjectInterface.h"

class UGameObject;
class UPhysicsSystem;

/// <summary>
/// RigidBodyComponent: 게임플레이 로직과 물리 시스템 간의 어댑터
/// 
/// 역할:
/// - 물리 상태의 캐시된 읽기 전용 접근 제공
/// - 모든 물리 명령을 Job으로 변환하여 PhysicsSystem에 전달
/// - ActorComponent 활성화 상태와 물리 시뮬레이션 자동 연동
/// 
/// 책임:
/// - IPhysicsState 인터페이스 구현 (Job 기반)
/// - IPhysicsObject 인터페이스 구현 (최신 시그니처)
/// - 캐시된 물리 상태 관리 및 동기화
/// - PhysicsSystem과의 생명주기 연동
/// 
/// 제약사항:
/// - 물리 상태 직접 수정 금지 (Job을 통해서만 가능)
/// - 시뮬레이션 중 캐시 수정 금지
/// - PhysicsID 유효성에 의존적
/// - 충돌 활성화는 CollisionComponent에서 독립 관리
/// - ActorComponent::SetActive()와 MASK_ACTIVATION 자동 연동
/// </summary>
class URigidBodyComponent : public USceneComponent,
	public IPhysicsState, public IPhysicsObject
{
#pragma region Constructor and Lifecycle(ActorComp)
public:
	URigidBodyComponent();
	~URigidBodyComponent();

	virtual void PostInitialized() override;

	// ActorComponent 활성화와 물리 시뮬레이션 자동 연동
	// UActorComponent::SetActive(bool)에 의해 자동 호출됨
	virtual void Activate() override;
	virtual void DeActivate() override;

	virtual void Tick(const float DeltaTime) override;

	virtual const char* GetComponentClassName() const override { return "URigid"; }
#pragma endregion

#pragma region SceneComponent Override
public:
	// SceneComponent override - Transform 변경을 물리 시스템에 전달
	void SetWorldTransform(const FTransform& InWorldTransform) override;
#pragma endregion

#pragma region IPhysicsState Implementation (Job-Based)
public:
	// === 운동 상태 설정 (Job 기반 - PhysicsID 전달) ===
	void SetVelocity(const Vector3& InVelocity) override;
	void AddVelocity(const Vector3& InVelocityDelta) override;
	void SetAngularVelocity(const Vector3& InAngularVelocity) override;
	void AddAngularVelocity(const Vector3& InAngularVelocityDelta) override;

	// === 힘 적용 (Job 기반 - PhysicsID 전달) ===
	inline void ApplyForce(const Vector3& Force) override { ApplyForce(Force, GetCenterOfMass()); }
	void ApplyForce(const Vector3& Force, const Vector3& Location) override;
	inline void ApplyImpulse(const Vector3& Impulse) { ApplyImpulse(Impulse, GetCenterOfMass()); }
	void ApplyImpulse(const Vector3& Impulse, const Vector3& Location) override;

	// === 즉시 읽기 가능한 캐시된 상태 ===
	inline Vector3 GetVelocity() const override { return CachedPhysicsState.Velocity; }
	inline Vector3 GetAngularVelocity() const override { return CachedPhysicsState.AngularVelocity; }

	inline float GetMass() const override
	{
		float invMass = GetInvMass();
		return invMass > 0.0f ? 1.0f / invMass : KINDA_LARGE;
	}
	inline float GetInvMass() const override
	{
		return IsStatic() ? 0.0f : CachedPhysicsState.InvMass;
	}

	inline Vector3 GetRotationalInertia() const override
	{
		Vector3 Result;
		Vector3 InvRotationalInertia = CachedPhysicsState.InvRotationalInertia;
		Result.x = (abs(InvRotationalInertia.x) < KINDA_SMALL) ?
			KINDA_LARGE : (1.0f / InvRotationalInertia.x);
		Result.y = (abs(InvRotationalInertia.y) < KINDA_SMALL) ?
			KINDA_LARGE : (1.0f / InvRotationalInertia.y);
		Result.z = (abs(InvRotationalInertia.z) < KINDA_SMALL) ?
			KINDA_LARGE : (1.0f / InvRotationalInertia.z);
		return Result;
	}
	inline Vector3 GetInvRotationalInertia() const override { return CachedPhysicsState.InvRotationalInertia; }

	inline float GetRestitution() const override { return CachedPhysicsState.Restitution; }
	inline float GetFrictionKinetic() const override { return CachedPhysicsState.FrictionKinetic; }
	inline float GetFrictionStatic() const override { return CachedPhysicsState.FrictionStatic; }

	// === 편의 메서드 ===
	inline float GetSpeed() const { return CachedPhysicsState.Velocity.Length(); }
#pragma endregion

#pragma region IPhysicsObject Implementation (Latest Interface)
public:
	// === 물리 시스템과의 동기화 ===
	void SynchronizeCachedStateFromSimulated() override;

	// === 생명주기 관리 ===
	void RegisterPhysicsSystem() override;
	void UnRegisterPhysicsSystem() override;

	// === 물리 시뮬레이션 (더 이상 직접 계산하지 않음) ===
	virtual void TickPhysics(const float DeltaTime) override;

	// === 물리 상태 및 식별자 ===
	PhysicsID GetPhysicsID() const override { return PhysicsID; }
	void SetPhysicsID(PhysicsID InID) override { PhysicsID = InID; }

	// === 최신 인터페이스: FPhysicsMask 반환 ===
	FPhysicsMask GetPhysicsMask() const override { return CachedPhysicsState.PhysicsMasks; }
#pragma endregion

#pragma region Physics Property Settings (Job-Based)
public:
	//물리 속성 기본값으로 초기화 설정
	void ResetPhysicsStates();

	// === 물리 속성 설정 (Job 기반) ===
	void SetMass(float InMass);
	void SetFrictionKinetic(float InFriction);
	void SetFrictionStatic(float InFriction);
	void SetRestitution(float InRestitution);
	void SetInvRotationalInertia(const Vector3& Value);

	// === 제한값 설정 (Job 기반) ===
	void SetMaxSpeed(float InSpeed);
	void SetMaxAngularSpeed(float InSpeed);
	void SetGravityScale(float InScale);

	// === PhysicsMask 기반 상태 제어 (Job 기반) ===
	void SetPhysicsType(EPhysicsType InType);
	void SetGravityEnabled(bool bEnabled);
	// 주의: 물리 활성화는 UActorComponent::SetActive()와 자동 연동됨
#pragma endregion
#pragma region Physics State Queries (Cache-Based)
public:
	// === PhysicsMask 기반 상태 조회 (캐시에서 즉시 읽기) ===
	bool IsGravityEnabled() const { return CachedPhysicsState.PhysicsMasks.HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED); }
	bool IsPhysicsActive() const { return CachedPhysicsState.PhysicsMasks.HasFlag(FPhysicsMask::MASK_ACTIVATION); }
	bool IsStatic() const { return CachedPhysicsState.PhysicsType == EPhysicsType::Static; }
	bool IsDynamic() const { return CachedPhysicsState.PhysicsType == EPhysicsType::Dynamic; }
	EPhysicsType GetPhysicsType() const { return CachedPhysicsState.PhysicsType; }
#pragma endregion

#pragma region Private Helpers and Internal Methods
private:
	// === 헬퍼 메서드 ===
	Vector3 GetCenterOfMass() const;

	// === Job 요청 헬퍼 (템플릿으로 타입 안전성 보장) ===
	template<typename JobType, typename... Args>
	void RequestPhysicsJob(Args&&... args);

	// === 물리 시스템 연동 내부 메서드 ===
	void UpdatePhysicsActivationState();
#pragma endregion

#pragma region Member Variables
private:
	// === 물리 시스템 식별자 ===
	PhysicsID PhysicsID = 0;

	// === 캐시된 물리 상태 (읽기 전용) ===
	FPhysicsState CachedPhysicsState;

	// === 상태 플래그 ===
	bool bIsRegisteredToPhysicsSystem = false;
#pragma endregion

};