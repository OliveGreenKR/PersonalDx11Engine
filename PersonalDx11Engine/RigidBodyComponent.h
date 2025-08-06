#pragma once
#include "Math.h"
#include <memory>
#include "ActorComponent.h"
#include "SceneComponent.h"
#include "PhysicsObjectInterface.h"
#include "PhysicsDataStructures.h"
#include "PhysicsDefine.h"
#include "FixedCircularQueue.h"
#include "CollisionDefines.h"

class UGameObject;
class UPhysicsSystem;
class UCollisionComponentBase;

using PhysicsID = std::uint32_t;

/// <summary>
/// RigidBodyComponent: 물리 시스템의 핵심 컴포넌트
/// 
/// 역할:
/// - 게임 상태 데이터 소유 및 관리 (Transform, Properties, Type/Mask)
/// - 물리 결과 캐시 및 게임 로직 접근 제공
/// - 더티 플래그 기반 효율적 동기화 지원
/// - CollisionComponent 관리 및 형상 정보 위임
/// - 충돌 이벤트 수신 및 전달
/// 
/// 새로운 설계 특징:
/// - CollisionComponent 의존성 관리
/// - 형상 정보 위임 처리
/// - 물리 상태와 형상 정보 분리
/// </summary>
class URigidBodyComponent : public USceneComponent, public IPhysicsObject
{
#pragma region Unit Conversion

private:
    constexpr static float UNIT_TO_METER = 0.01f;
    constexpr static float METER_TO_UNIT = 100.0f;

#pragma endregion

#pragma region Constructor and Lifecycle

public:
    URigidBodyComponent();
    ~URigidBodyComponent();

    virtual void PostInitialized() override;
    virtual void PostTreeInitialized() override;
    virtual void Tick(const float DeltaTime) override;

    virtual void Activate() override;
    virtual void DeActivate() override;

    virtual const char* GetComponentClassName() const override { return "URigidBody"; }

#pragma endregion

#pragma region State Data Management

private:
    FHighFrequencyData HighFrequencyGameState;
    FMidFrequencyData MidFrequencyGameState;
    FLowFrequencyData LowFrequencyGameState;

    FPhysicsToGameData PhysicsResultCache;
    FPhysicsDataDirtyFlags DirtyFlags = FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_ALL);

    PhysicsID PhysicsObjectID = 0;
    bool bIsRegisteredToPhysicsSystem = false;

    Vector3 PreviousPosition = Vector3::Zero();
    Quaternion PreviousRotation = Quaternion::Identity();

    bool bEnableTimeInterpolation = true;
    float InterpolationAlpha = 0.0f;
    float LastTickTime = 0.0f;

#pragma endregion

#pragma region IPhysicsObject Implementation

public:
    FHighFrequencyData GetHighFrequencyData() override;
    FMidFrequencyData GetMidFrequencyData() override;
    FLowFrequencyData GetLowFrequencyData() override;

    void ReceivePhysicsResults(const FPhysicsToGameData& results) override;

    FPhysicsDataDirtyFlags GetDirtyFlags() const override;
    void MarkDataClean(const FPhysicsDataDirtyFlags& flags) override;

    void RegisterPhysicsSystem() override;
    void UnRegisterPhysicsSystem() override;

    PhysicsID GetPhysicsID() const override;
    FPhysicsMask GetPhysicsMask() const override;

#pragma endregion

#pragma region Game Logic Interface

public:
    void SetPhysicsType(EPhysicsType InType);
    void SetGravityEnabled(bool bEnabled);
    void SetPhysicsActive(bool bActive);

    void SetMass(float InMass);
    void SetInvRotationalInertia(const Vector3& InInvInertia);
    void SetRestitution(float InRestitution);
    void SetFrictionStatic(float InFriction);
    void SetFrictionKinetic(float InFriction);
    void SetMaxSpeed(float InMaxSpeed);
    void SetMaxAngularSpeed(float InMaxAngularSpeed);
    void SetGravityScale(float InGravityScale);

    float GetMass() const;
    float GetInvMass() const;
    Vector3 GetRotationalInertia() const;
    Vector3 GetInvRotationalInertia() const;
    float GetRestitution() const;
    float GetFrictionStatic() const;
    float GetFrictionKinetic() const;
    float GetMaxSpeed() const;
    float GetMaxAngularSpeed() const;
    float GetGravityScale() const;
    float GetSpeed() const;

    EPhysicsType GetPhysicsType() const;
    bool IsStatic() const;
    bool IsDynamic() const;
    bool IsGravityEnabled() const;
    bool IsPhysicsActive() const;

    Vector3 GetVelocity() const;
    Vector3 GetAngularVelocity() const;

    void SetVelocity(const Vector3& InVelocity);
    void AddVelocity(const Vector3& InVelocityDelta);
    void SetAngularVelocity(const Vector3& InAngularVelocity);
    void AddAngularVelocity(const Vector3& InAngularVelocityDelta);

    void SetWorldTransform(const FTransform& InWorldTransform) override;

    void ApplyForce(const Vector3& InForce);
    void ApplyForce(const Vector3& InForce, const Vector3& InLocation);
    void ApplyImpulse(const Vector3& InImpulse);
    void ApplyImpulse(const Vector3& InImpulse, const Vector3& InLocation);

#pragma endregion

#pragma region Event Receiving System & CollisionComponent Management

public:
    /// <summary>
    /// 물리 시스템으로부터 원본 물리 충돌 이벤트를 배치로 수신
    /// </summary>
    void ReceiveCollisionEvents(std::vector<FPhysicsCollisionEvent>& PhysicsEvents) override;

    /// <summary>
    /// CollisionComponent 설정 및 관리
    /// </summary>
    void SetCollisionComp(UCollisionComponentBase* InCollisionComp);
    UCollisionComponentBase* GetCollisionComp() const;
    bool HasCollisionComp() const;

private:
    FCollisionEvent ConvertPhysicsToGameEvent(const FPhysicsCollisionEvent& PhysicsEvent);
    void DispatchToOwnCollisionComponents(const FCollisionEvent& GameEvent);

    UCollisionComponentBase* OwnComponent = nullptr;

#pragma endregion

#pragma region Time Interpolation

public:
    void SetTimeInterpolationEnabled(bool bEnabled);
    bool IsTimeInterpolationEnabled() const;

private:
    void ApplyInterporateTransform(const FPhysicsToGameData& PhysicsResults, const FTransform& CurrentGameTransform);

#pragma endregion

#pragma region Utility Methods

private:
    void InitializeGameState();
    void InitializePhysicsCache();
    void MarkDataDirty(const FPhysicsDataDirtyFlags& flags);
    void OnWorldTransformChanged(const FTransform& transform);

#pragma endregion

};