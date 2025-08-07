#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "Debug.h"
#include "PhysicsDefine.h"
#include "PhysicsSystem.h"
#include "PhysicsJob.h"
#include "ConfigReadManager.h"
#include "SceneManager.h"
#include "CollisionComponent.h"
#include "TypeCast.h"

#pragma region Constructor and Lifecycle

URigidBodyComponent::URigidBodyComponent()
{
    bPhysicsSimulated = true;
    InitializeGameState();
    InitializePhysicsCache();
	BackupCurrentTransformToPrevious();
}

URigidBodyComponent::~URigidBodyComponent()
{
    if (bIsRegisteredToPhysicsSystem)
    {
        UnRegisterPhysicsSystem();
    }
}

void URigidBodyComponent::PostInitialized()
{
    USceneComponent::PostInitialized();

    FTransform currentTransform = USceneComponent::GetWorldTransform();
    HighFrequencyGameState = FHighFrequencyData(currentTransform);
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));

    OnWorldTransformChangedDelegate.Bind(this, [this](const FTransform& transform) {
        OnWorldTransformChanged(transform);
                                         }, "OnTransformChanged_Rigid");
}

void URigidBodyComponent::PostTreeInitialized()
{
    USceneComponent::PostTreeInitialized();

    // CollisionComponent 자동 탐지 및 설정
    auto collisionComp = FindChildByType<UCollisionComponentBase>();
    if (collisionComp.lock())
    {
        SetCollisionComp(collisionComp.lock().get());
    }

    if (!bIsRegisteredToPhysicsSystem)
    {
        RegisterPhysicsSystem();
    }
}

void URigidBodyComponent::Activate()
{
    USceneComponent::Activate();

    if (!bIsRegisteredToPhysicsSystem)
    {
        RegisterPhysicsSystem();
    }

    SetPhysicsActive(true);
}

void URigidBodyComponent::DeActivate()
{
    USceneComponent::DeActivate();
    SetPhysicsActive(false);
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
    USceneComponent::Tick(DeltaTime);

    if (!IsActive())
        return;
}

#pragma endregion

#pragma region IPhysicsObject Implementation - Physics System Lifecycle

void URigidBodyComponent::RegisterPhysicsSystem()
{
    if (bIsRegisteredToPhysicsSystem)
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
    {
        LOG_ERROR("URigidBodyComponent::RegisterPhysicsSystem - PhysicsSystem not available");
        return;
    }

    std::shared_ptr<IPhysicsObject> PhysicsObjectPtr = Engine::Cast<IPhysicsObject>(
        Engine::Cast<URigidBodyComponent>(shared_from_this()));
    PhysicsObjectID = PhysicsSystem->RegisterPhysicsObject(PhysicsObjectPtr);

    if (PhysicsObjectID != 0)
    {
        bIsRegisteredToPhysicsSystem = true;
        LOG_INFO("URigidBodyComponent registered to PhysicsSystem with ID: %u", PhysicsObjectID);
    }
    else
    {
        LOG_ERROR("URigidBodyComponent::RegisterPhysicsSystem - Failed to register");
    }
}

void URigidBodyComponent::UnRegisterPhysicsSystem()
{
    if (!bIsRegisteredToPhysicsSystem)
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem && PhysicsObjectID != 0)
    {
        PhysicsSystem->UnregisterPhysicsObject(PhysicsObjectID);
        LOG_INFO("URigidBodyComponent unregistered from PhysicsSystem with ID: %u", PhysicsObjectID);
    }

    PhysicsObjectID = 0;
    bIsRegisteredToPhysicsSystem = false;
}

PhysicsID URigidBodyComponent::GetPhysicsID() const
{
    return PhysicsObjectID;
}

FPhysicsMask URigidBodyComponent::GetPhysicsMask() const
{
    return MidFrequencyGameState.PhysicsMask;
}

#pragma endregion

#pragma region IPhysicsObject Implementation - Data Providers

FHighFrequencyData URigidBodyComponent::GetHighFrequencyData()
{
    FTransform CurrentTransform = GetWorldTransform();

    // 내부 상태 업데이트 (게임 좌표계)
    HighFrequencyGameState.SetPrevTransform(PreviousTransform);
    HighFrequencyGameState.SetTransform(CurrentTransform);

    // 내부 상태 복사 후 물리 시스템용 단위 변환
    FHighFrequencyData ToTransfer = HighFrequencyGameState;
    ToTransfer.Position *= UNIT_TO_METER;
    ToTransfer.PrevPosition *= UNIT_TO_METER;
    // 회전과 스케일은 단위 변환 불필요

    return ToTransfer;
}

FMidFrequencyData URigidBodyComponent::GetMidFrequencyData()
{
    return MidFrequencyGameState;
}

FLowFrequencyData URigidBodyComponent::GetLowFrequencyData()
{
    FLowFrequencyData result = LowFrequencyGameState;

    // CollisionComponent 존재 여부에 따른 형상 데이터 설정
    if (OwnComponent && OwnComponent->IsActive()) {
        result.CollisionShapeType = OwnComponent->GetType();
        // 월드 크기 반영된 형상 크기를 물리 시스템 단위로 변환
        result.CollisionWorldHalfExtent = OwnComponent->GetScaledHalfExtent() * UNIT_TO_METER;
    }
    else {
        // CollisionComponent가 없거나 비활성화된 경우
        // CollisionComponent가 없거나 비활성화된 경우
        result.CollisionShapeType = ECollisionShapeType::None;
        result.CollisionWorldHalfExtent = Vector3::Zero();
    }

    // 물리 시스템용 단위 변환
    result.MaxSpeed = result.MaxSpeed * UNIT_TO_METER;

    return result;
}

#pragma endregion

#pragma region IPhysicsObject Implementation - Physics Results Reception

void URigidBodyComponent::ReceivePhysicsResults(const FPhysicsToGameData& results)
{
    PhysicsResultCache = results;
    PhysicsResultCache.ResultPosition = PhysicsResultCache.ResultPosition * METER_TO_UNIT;
    PhysicsResultCache.Velocity = PhysicsResultCache.Velocity * METER_TO_UNIT;

    FTransform CurrentGameTransform = GetWorldTransform();
    FTransform PhysicsResultTransform = FTransform(PhysicsResultCache.ResultPosition,
                                                   PhysicsResultCache.ResultRotation,
                                                   PhysicsResultCache.ResultScale);

    if (FTransform::IsEqual(CurrentGameTransform, PhysicsResultTransform))
    {
        return;
    }

    ApplyInterporateTransform(PhysicsResultCache, CurrentGameTransform);

    HighFrequencyGameState.Position = PhysicsResultCache.ResultPosition;
    HighFrequencyGameState.Rotation = PhysicsResultCache.ResultRotation;
    HighFrequencyGameState.Scale = PhysicsResultCache.ResultScale;
}

#pragma endregion

#pragma region IPhysicsObject Implementation - Dirty Flag Management

FPhysicsDataDirtyFlags URigidBodyComponent::GetDirtyFlags() const
{
    return DirtyFlags;
}

void URigidBodyComponent::MarkDataClean(const FPhysicsDataDirtyFlags& flags)
{
    DirtyFlags.ClearFlag(flags.GetRawFlags());
}

#pragma endregion

#pragma region Event Receiving System & CollisionComponent Management

void URigidBodyComponent::ReceiveCollisionEvents(std::vector<FPhysicsCollisionEvent>& PhysicsEvents)
{
    for (const auto& PhysicsEvent : PhysicsEvents)
    {
        FCollisionEvent GameEvent = ConvertPhysicsToGameEvent(PhysicsEvent);
        DispatchToOwnCollisionComponents(GameEvent);
    }
}

FCollisionEvent URigidBodyComponent::ConvertPhysicsToGameEvent(const FPhysicsCollisionEvent& PhysicsEvent)
{
    FCollisionEvent GameEvent;

    GameEvent.CollisionState = PhysicsEvent.CollisionState;
    GameEvent.Other = nullptr;

    GameEvent.CollisionPoint = Vector3(
        XMVectorGetX(PhysicsEvent.CollisionPoint) * METER_TO_UNIT,
        XMVectorGetY(PhysicsEvent.CollisionPoint) * METER_TO_UNIT,
        XMVectorGetZ(PhysicsEvent.CollisionPoint) * METER_TO_UNIT
    );

    GameEvent.Normal = Vector3(
        XMVectorGetX(PhysicsEvent.Normal),
        XMVectorGetY(PhysicsEvent.Normal),
        XMVectorGetZ(PhysicsEvent.Normal)
    );

    GameEvent.PenetrationDepth = PhysicsEvent.PenetrationDepth * METER_TO_UNIT;

    return GameEvent;
}

void URigidBodyComponent::DispatchToOwnCollisionComponents(const FCollisionEvent& GameEvent)
{
    if (!OwnComponent)
        return;

    switch (GameEvent.CollisionState)
    {
        case ECollisionState::Enter:
            OwnComponent->OnCollisionEnterEvent(GameEvent);
            break;
        case ECollisionState::Stay:
            OwnComponent->OnCollisionStayEvent(GameEvent);
            break;
        case ECollisionState::Exit:
            OwnComponent->OnCollisionExitEvent(GameEvent);
            break;
    }
}

void URigidBodyComponent::SetCollisionComp(UCollisionComponentBase* InCollisionComp)
{
	constexpr const char* OnCollisionChangedName = "OnTransformChanged_Rigid";

    if (OwnComponent != InCollisionComp)
    {
        if(OwnComponent)
        {
            // 기존 CollisionComponent가 있다면 이벤트 언바인딩
			OwnComponent->OnWorldTransformChangedDelegate.Unbind(this, OnCollisionChangedName);
        }

        OwnComponent = InCollisionComp;
        if (OwnComponent)
        {
            // 새로운 CollisionComponent가 있다면 이벤트 바인딩
            OwnComponent->OnWorldTransformChangedDelegate.Bind(this, &URigidBodyComponent::OnCollisionComponentChanged, OnCollisionChangedName);
		}

        // CollisionComponent 변경 시 형상 데이터 변경으로 인한 DirtyFlag 설정
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));

        if (OwnComponent)
        {
            LOG_INFO("CollisionComponent set for RigidBodyComponent with PhysicsID: %u", PhysicsObjectID);
        }
        else
        {
            LOG_INFO("CollisionComponent removed from RigidBodyComponent with PhysicsID: %u", PhysicsObjectID);
        }
    }
}

UCollisionComponentBase* URigidBodyComponent::GetCollisionComp() const
{
    return OwnComponent;
}

bool URigidBodyComponent::HasCollisionComp() const
{
    return OwnComponent != nullptr;
}

#pragma endregion

#pragma region Time Interpolation System

void URigidBodyComponent::SetTimeInterpolationEnabled(bool bEnabled)
{
    bEnableTimeInterpolation = bEnabled;
}

bool URigidBodyComponent::IsTimeInterpolationEnabled() const
{
    return bEnableTimeInterpolation;
}


void URigidBodyComponent::ApplyInterporateTransform(const FPhysicsToGameData& PhysicsResults, const FTransform& CurrentGameTransform)
{
    if (!bEnableTimeInterpolation)
    {
        // 보간 비활성화 시 물리 결과 직접 적용
        SetWorldTransform(FTransform(PhysicsResults.ResultPosition,
                                     PhysicsResults.ResultRotation,
                                     PhysicsResults.ResultScale));
        return;
    }

    // SIMD 최적화된 변화량 계산
    XMVECTOR CurrentPos = XMLoadFloat3(&CurrentGameTransform.Position);
    XMVECTOR PreviousPos = XMLoadFloat3(&PreviousTransform.Position);
    XMVECTOR PhysicsPos = XMLoadFloat3(&PhysicsResults.ResultPosition);

    XMVECTOR CurrentRot = XMLoadFloat4(&CurrentGameTransform.Rotation);
    XMVECTOR PreviousRot = XMLoadFloat4(&PreviousTransform.Rotation);
    XMVECTOR PhysicsRot = XMLoadFloat4(&PhysicsResults.ResultRotation);

    // 변화량 분리 (Previous 기준으로 SIMD 계산)
    XMVECTOR GameLogicDelta = XMVectorSubtract(CurrentPos, PreviousPos);
    XMVECTOR PhysicsDelta = XMVectorSubtract(PhysicsPos, PreviousPos);

    // 회전 변화량 계산 (SIMD 최적화)
    XMVECTOR PreviousRotInv = XMQuaternionInverse(PreviousRot);
    XMVECTOR GameLogicRotDelta = XMQuaternionMultiply(CurrentRot, PreviousRotInv);
    XMVECTOR PhysicsRotDelta = XMQuaternionMultiply(PhysicsRot, PreviousRotInv);

    // 단순 가중치 계산 (기본값: 물리 70%)
    float PhysicsWeight = 0.7f;
    XMVECTOR WeightVector = XMVectorReplicate(PhysicsWeight);

    // 최종 위치 계산 (SIMD)
    // 게임 로직 변화는 100% 반영, 물리 변화는 가중치 적용
    XMVECTOR WeightedPhysicsDelta = XMVectorMultiply(PhysicsDelta, WeightVector);
    XMVECTOR FinalPosVector = XMVectorAdd(XMVectorAdd(PreviousPos, GameLogicDelta), WeightedPhysicsDelta);

    // 회전은 DirectXMath의 최적화된 Slerp 사용
    XMVECTOR FinalRotVector = XMQuaternionSlerp(CurrentRot, PhysicsRot, PhysicsWeight);
    FinalRotVector = XMQuaternionNormalize(FinalRotVector);

    // 결과 저장
    Vector3 FinalPosition;
    Quaternion FinalRotation;
    XMStoreFloat3(&FinalPosition, FinalPosVector);
    XMStoreFloat4(&FinalRotation, FinalRotVector);

    // 스케일은 게임 로직 우선
    Vector3 FinalScale = CurrentGameTransform.Scale;

    FTransform FinalTransform = FTransform(FinalPosition, FinalRotation, FinalScale);
    SetWorldTransform(FinalTransform);

    // 디버그 로그 (임시) - SIMD 계산된 차이값 활용
    XMVECTOR DifferenceVector = XMVectorSubtract(CurrentPos, PhysicsPos);
    float PositionDifference = XMVectorGetX(XMVector3Length(DifferenceVector));
    if (PositionDifference > 1.0f)
    {
        LOG_INFO("SIMD Transform: Game[%s] \n Physics[%s] \n Final[%s]",
                 Debug::ToString(CurrentGameTransform.Position),
                 Debug::ToString(PhysicsResults.ResultPosition),
                 Debug::ToString(FinalPosition));
    }
}
#pragma endregion

#pragma region Game Logic Interface - Physics Type and State Management

void URigidBodyComponent::SetPhysicsType(EPhysicsType InType)
{
    if (MidFrequencyGameState.PhysicsType != InType)
    {
        MidFrequencyGameState.PhysicsType = InType;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_MID_FREQ));
    }
}

void URigidBodyComponent::SetGravityEnabled(bool bEnabled)
{
    bool CurrentGravityState = MidFrequencyGameState.PhysicsMask.HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    if (CurrentGravityState != bEnabled)
    {
        if (bEnabled)
        {
            MidFrequencyGameState.PhysicsMask.SetFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
        }
        else
        {
            MidFrequencyGameState.PhysicsMask.ClearFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
        }
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_MID_FREQ));
    }
}

void URigidBodyComponent::SetPhysicsActive(bool bActive)
{
    bool CurrentActiveState = MidFrequencyGameState.PhysicsMask.HasFlag(FPhysicsMask::MASK_ACTIVATION);
    if (CurrentActiveState != bActive)
    {
        if (bActive)
        {
            MidFrequencyGameState.PhysicsMask.SetFlag(FPhysicsMask::MASK_ACTIVATION);
        }
        else
        {
            MidFrequencyGameState.PhysicsMask.ClearFlag(FPhysicsMask::MASK_ACTIVATION);
        }
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_MID_FREQ));
    }
}

EPhysicsType URigidBodyComponent::GetPhysicsType() const
{
    return MidFrequencyGameState.PhysicsType;
}

bool URigidBodyComponent::IsStatic() const
{
    return MidFrequencyGameState.PhysicsType == EPhysicsType::Static;
}

bool URigidBodyComponent::IsDynamic() const
{
    return MidFrequencyGameState.PhysicsType == EPhysicsType::Dynamic;
}

bool URigidBodyComponent::IsGravityEnabled() const
{
    return MidFrequencyGameState.PhysicsMask.HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
}

bool URigidBodyComponent::IsPhysicsActive() const
{
    return MidFrequencyGameState.PhysicsMask.HasFlag(FPhysicsMask::MASK_ACTIVATION);
}

#pragma endregion

#pragma region Game Logic Interface - Physics Properties Management

void URigidBodyComponent::SetMass(float InMass)
{
    float NewInvMass = (InMass > 0.0f) ? (1.0f / InMass) : 0.0f;
    if (LowFrequencyGameState.InvMass != NewInvMass)
    {
        LowFrequencyGameState.InvMass = NewInvMass;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetInvRotationalInertia(const Vector3& InInvInertia)
{
    if (LowFrequencyGameState.InvRotationalInertia != InInvInertia)
    {
        LowFrequencyGameState.InvRotationalInertia = InInvInertia;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetRestitution(float InRestitution)
{
    if (LowFrequencyGameState.Restitution != InRestitution)
    {
        LowFrequencyGameState.Restitution = InRestitution;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetFrictionStatic(float InFriction)
{
    if (LowFrequencyGameState.FrictionStatic != InFriction)
    {
        LowFrequencyGameState.FrictionStatic = InFriction;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetFrictionKinetic(float InFriction)
{
    if (LowFrequencyGameState.FrictionKinetic != InFriction)
    {
        LowFrequencyGameState.FrictionKinetic = InFriction;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetMaxSpeed(float InMaxSpeed)
{
    if (LowFrequencyGameState.MaxSpeed != InMaxSpeed)
    {
        LowFrequencyGameState.MaxSpeed = InMaxSpeed;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetMaxAngularSpeed(float InMaxAngularSpeed)
{
    if (LowFrequencyGameState.MaxAngularSpeed != InMaxAngularSpeed)
    {
        LowFrequencyGameState.MaxAngularSpeed = InMaxAngularSpeed;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetGravityScale(float InGravityScale)
{
    if (LowFrequencyGameState.GravityScale != InGravityScale)
    {
        LowFrequencyGameState.GravityScale = InGravityScale;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

// Properties 조회 함수들
float URigidBodyComponent::GetMass() const
{
    return LowFrequencyGameState.GetMass();
}

float URigidBodyComponent::GetInvMass() const
{
    return LowFrequencyGameState.InvMass;
}

Vector3 URigidBodyComponent::GetRotationalInertia() const
{
    return LowFrequencyGameState.GetRotationalInertia();
}

Vector3 URigidBodyComponent::GetInvRotationalInertia() const
{
    return LowFrequencyGameState.InvRotationalInertia;
}

float URigidBodyComponent::GetRestitution() const
{
    return LowFrequencyGameState.Restitution;
}

float URigidBodyComponent::GetFrictionStatic() const
{
    return LowFrequencyGameState.FrictionStatic;
}

float URigidBodyComponent::GetFrictionKinetic() const
{
    return LowFrequencyGameState.FrictionKinetic;
}

float URigidBodyComponent::GetMaxSpeed() const
{
    return LowFrequencyGameState.MaxSpeed;
}

float URigidBodyComponent::GetMaxAngularSpeed() const
{
    return LowFrequencyGameState.MaxAngularSpeed;
}

float URigidBodyComponent::GetGravityScale() const
{
    return LowFrequencyGameState.GravityScale;
}

float URigidBodyComponent::GetSpeed() const
{
    return PhysicsResultCache.Velocity.Length();
}

#pragma endregion

#pragma region Game Logic Interface - Physics State Queries

Vector3 URigidBodyComponent::GetVelocity() const
{
    return PhysicsResultCache.Velocity;
}

Vector3 URigidBodyComponent::GetAngularVelocity() const
{
    return PhysicsResultCache.AngularVelocity;
}

#pragma endregion

#pragma region Game Logic Interface - Immediate Physics Commands

void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetVelocity - Component not registered to physics system");
        return;
    }
    if (!IsActive() || IsStatic())
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        Vector3 ConvertedVelocity = InVelocity * UNIT_TO_METER;
        PhysicsSystem->RequestPhysicsJob<FJobSetVelocity>(PhysicsObjectID, ConvertedVelocity);
    }
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::AddVelocity - Component not registered to physics system");
        return;
    }
    if (!IsActive() || IsStatic())
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        Vector3 ConvertedVelocityDelta = InVelocityDelta * UNIT_TO_METER;
        PhysicsSystem->RequestPhysicsJob<FJobAddVelocity>(PhysicsObjectID, ConvertedVelocityDelta);
    }
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetAngularVelocity - Component not registered to physics system");
        return;
    }
    if (!IsActive() || IsStatic())
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetAngularVelocity>(PhysicsObjectID, InAngularVelocity);
    }
}

void URigidBodyComponent::AddAngularVelocity(const Vector3& InAngularVelocityDelta)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::AddAngularVelocity - Component not registered to physics system");
        return;
    }
    if (!IsActive() || IsStatic())
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobAddAngularVelocity>(PhysicsObjectID, InAngularVelocityDelta);
    }
}

void URigidBodyComponent::ApplyForce(const Vector3& InForce)
{
    // 질량 중심점에 힘 적용
    Vector3 CenterOfMass = GetWorldTransform().Position; // 간단화: Transform 위치를 질량 중심으로 사용
    ApplyForce(InForce, CenterOfMass);
}

void URigidBodyComponent::ApplyForce(const Vector3& InForce, const Vector3& InLocation)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::ApplyForce - Component not registered to physics system");
        return;
    }
    if (!IsActive() || IsStatic())
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        Vector3 ConvertedForce = InForce;
        Vector3 ConvertedLocation = InLocation * UNIT_TO_METER;
        PhysicsSystem->RequestPhysicsJob<FJobApplyForce>(PhysicsObjectID, ConvertedForce, ConvertedLocation);
    }
}

void URigidBodyComponent::ApplyImpulse(const Vector3& InImpulse)
{
    // 질량 중심점에 충격 적용
    Vector3 CenterOfMass = GetWorldTransform().Position; // 간단화: Transform 위치를 질량 중심으로 사용
    ApplyImpulse(InImpulse, CenterOfMass);
}

void URigidBodyComponent::ApplyImpulse(const Vector3& InImpulse, const Vector3& InLocation)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::ApplyImpulse - Component not registered to physics system");
        return;
    }
    if (!IsActive() || IsStatic())
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        Vector3 ConvertedImpulse = InImpulse;
        Vector3 ConvertedLocation = InLocation * UNIT_TO_METER;
        PhysicsSystem->RequestPhysicsJob<FJobApplyImpulse>(PhysicsObjectID, ConvertedImpulse, ConvertedLocation);
    }
}

#pragma endregion

#pragma region Utility Methods
void URigidBodyComponent::InitializeGameState()
{
    // High Frequency 초기화 (Transform)
    HighFrequencyGameState = FHighFrequencyData();

    // Mid Frequency 초기화 (Type, Mask)
    MidFrequencyGameState.PhysicsType = EPhysicsType::Dynamic;
    MidFrequencyGameState.PhysicsMask = FPhysicsMask(FPhysicsMask::GROUP_BASIC_SIMULATION);

    // Low Frequency 초기화 (Properties)
    LowFrequencyGameState = FLowFrequencyData(); // 기본값으로 초기화

    // 모든 데이터가 더티 상태로 시작
    DirtyFlags = FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_ALL);
}

void URigidBodyComponent::InitializePhysicsCache()
{
    // 물리 결과 캐시 초기화
    PhysicsResultCache.Velocity = Vector3::Zero();
    PhysicsResultCache.AngularVelocity = Vector3::Zero();
    PhysicsResultCache.ResultPosition = Vector3::Zero();
    PhysicsResultCache.ResultRotation = Quaternion::Identity();
    PhysicsResultCache.ResultScale = Vector3::One();
}

void URigidBodyComponent::MarkDataDirty(const FPhysicsDataDirtyFlags& flags)
{
    DirtyFlags |= flags;
}
inline void URigidBodyComponent::BackupCurrentTransformToPrevious()
{
    // 현재 Transform을 Previous로 백업
    PreviousTransform = GetWorldTransform();
}
#pragma endregion

#pragma region EventHandlers
void URigidBodyComponent::OnWorldTransformChanged(const FTransform& NewTransform)
{
    // 더티 플래그 설정
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));
    // 이전 트랜스폼 백업
	BackupCurrentTransformToPrevious();
}

void URigidBodyComponent::OnCollisionComponentChanged(const FTransform& transform)
{
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
}
#pragma endregion