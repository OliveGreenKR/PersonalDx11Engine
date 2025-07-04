// RigidBodyComponent.h
#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsObjectInterface.h"
#include "PhysicsDataStructures.h"
#include "PhysicsDefine.h"

class UGameObject;
class UPhysicsSystem;

/// <summary>
/// RigidBodyComponent: 새로운 하이브리드 물리 시스템의 핵심 컴포넌트
/// 
/// 역할:
/// - 게임 상태 데이터 소유 및 관리 (Transform, Properties, Type/Mask)
/// - 물리 결과 캐시 및 게임 로직 접근 제공
/// - 더티 플래그 기반 효율적 동기화 지원
/// - Job 시스템을 통한 즉시 물리 명령 전송
/// 
/// 새로운 설계 특징:
/// - 변경 빈도별 데이터 분리 관리
/// - 배치 동기화 시스템 지원
/// - 게임 상태 즉시 반영 (물리 결과는 동기화 시점에 반영)
/// - Job과 동기화의 하이브리드 사용
/// 
/// 데이터 소유권:
/// - 게임 컨텍스트: Transform, Properties, Type, Mask (즉시 변경 가능)
/// - 물리 결과 캐시: Velocity, AngularVelocity 등 (동기화로 수신)
/// </summary>
class URigidBodyComponent : public USceneComponent, public IPhysicsObject
{
#pragma region Constructor and Lifecycle

public:
    URigidBodyComponent();
    ~URigidBodyComponent();

    virtual void PostInitialized() override;
    virtual void PostTreeInitialized() override;
    virtual void Tick(const float DeltaTime) override;

    // ActorComponent 활성화와 물리 시뮬레이션 자동 연동
    virtual void Activate() override;
    virtual void DeActivate() override;

    virtual const char* GetComponentClassName() const override { return "URigidBody"; }

#pragma endregion

#pragma region Game State Data Management (Game Ownership)

private:
    // === 게임 상태 데이터 (소유권: 게임 컨텍스트) ===
    FHighFrequencyData HighFrequencyGameState;    // Transform
    FMidFrequencyData MidFrequencyGameState;      // Type, Mask
    FLowFrequencyData LowFrequencyGameState;      // Properties

    // === 물리 결과 캐시 (소유권: 물리 시스템에서 수신) ===
    FPhysicsToGameData PhysicsResultCache;

    // === 더티 플래그 시스템 ===
    FPhysicsDataDirtyFlags DirtyFlags = FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_ALL);

    // === 물리 시스템 연동 ===
    PhysicsID PhysicsObjectID = 0;
    bool bIsRegisteredToPhysicsSystem = false;

#pragma endregion

#pragma region IPhysicsObject Implementation

public:
    // === 게임 상태 데이터 제공 (Game → Physics) ===
    FHighFrequencyData GetHighFrequencyData() const override;
    FMidFrequencyData GetMidFrequencyData() const override;
    FLowFrequencyData GetLowFrequencyData() const override;

    // === 물리 결과 수신 (Physics → Game) ===
    void ReceivePhysicsResults(const FPhysicsToGameData& results) override;

    // === 더티 플래그 관리 ===
    FPhysicsDataDirtyFlags GetDirtyFlags() const override;
    void MarkDataClean(const FPhysicsDataDirtyFlags& flags) override;

    // === 물리 시스템 생명주기 ===
    void RegisterPhysicsSystem() override;
    void UnRegisterPhysicsSystem() override;
    void TickPhysics(const float DeltaTime) override;

    // === 물리 시스템 통합 ===
    PhysicsID GetPhysicsID() const override;
    FPhysicsMask GetPhysicsMask() const override;

#pragma endregion

#pragma region Game Logic Interface (Immediate Updates)

public:
    // === Transform 설정 (High Frequency) ===
    void SetWorldTransform(const FTransform& InWorldTransform) override;
    void SetWorldPosition(const Vector3& InPosition);
    void SetWorldRotation(const Quaternion& InRotation);
    void SetWorldScale(const Vector3& InScale);

    // === Physics Type 및 Mask 설정 (Mid Frequency) ===
    void SetPhysicsType(EPhysicsType InType);
    void SetGravityEnabled(bool bEnabled);
    void SetPhysicsActive(bool bActive);

    // === Physics Properties 설정 (Low Frequency) ===
    void SetMass(float InMass);
    void SetFrictionKinetic(float InFriction);
    void SetFrictionStatic(float InFriction);
    void SetRestitution(float InRestitution);
    void SetInvRotationalInertia(const Vector3& InValue);
    void SetMaxSpeed(float InSpeed);
    void SetMaxAngularSpeed(float InSpeed);
    void SetGravityScale(float InScale);

    // === 물리 상태 조회 (캐시된 값) ===
    Vector3 GetVelocity() const;
    Vector3 GetAngularVelocity() const;
    float GetMass() const;
    float GetInvMass() const;
    Vector3 GetRotationalInertia() const;
    Vector3 GetInvRotationalInertia() const;
    float GetRestitution() const;
    float GetFrictionKinetic() const;
    float GetFrictionStatic() const;
    float GetSpeed() const;
    bool IsGravityEnabled() const;
    bool IsPhysicsActive() const;
    bool IsStatic() const;
    bool IsDynamic() const;
    EPhysicsType GetPhysicsType() const;

#pragma endregion

#pragma region Job-Based Physics Commands (Immediate Actions)

public:
    // === 힘/충격 적용 (Job 시스템 사용) ===
    void ApplyForce(const Vector3& Force);
    void ApplyForce(const Vector3& Force, const Vector3& Location);
    void ApplyImpulse(const Vector3& Impulse);
    void ApplyImpulse(const Vector3& Impulse, const Vector3& Location);

    // === 즉시 속도 변경 (Job 시스템 사용) ===
    void SetVelocity(const Vector3& InVelocity);
    void AddVelocity(const Vector3& InVelocityDelta);
    void SetAngularVelocity(const Vector3& InAngularVelocity);
    void AddAngularVelocity(const Vector3& InAngularVelocityDelta);

#pragma endregion

#pragma region Internal Helpers

private:
    /// <summary>
    /// 더티 플래그 설정 및 물리 시스템 업데이트 알림
    /// </summary>
    void MarkDataDirty(const FPhysicsDataDirtyFlags& flags);

    /// <summary>
    /// 물리 시스템 ID 설정 (등록 시 자동 호출)
    /// </summary>
    void SetPhysicsID(PhysicsID InID);

    /// <summary>
    /// 게임 상태 기본값으로 초기화
    /// </summary>
    void InitializeGameState();

    /// <summary>
    /// 물리 결과 캐시 기본값으로 초기화
    /// </summary>
    void InitializePhysicsCache();

    /// <summary>
    /// 질량 중심 계산
    /// </summary>
    Vector3 GetCenterOfMass() const;

#pragma endregion

};