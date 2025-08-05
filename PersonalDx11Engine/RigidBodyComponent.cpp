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
    InitializeTimeInterpolation();
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
    PreviousPosition = CurrentTransform.Position;
    PreviousRotation = CurrentTransform.Rotation;

    HighFrequencyGameState = FHighFrequencyData(CurrentTransform);

    FHighFrequencyData ToTransfer = HighFrequencyGameState;
    ToTransfer.Position *= UNIT_TO_METER;
    return ToTransfer;
}

FMidFrequencyData URigidBodyComponent::GetMidFrequencyData()
{
    return MidFrequencyGameState;
}

FLowFrequencyData URigidBodyComponent::GetLowFrequencyData()
{
    FLowFrequencyData result = LowFrequencyGameState;
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

#pragma region Event Receiving System

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
    OwnComponent = InCollisionComp;

    if (OwnComponent)
    {
        LOG_INFO("CollisionComponent set for RigidBodyComponent with PhysicsID: %u", PhysicsObjectID);
    }
}

#pragma endregion

#pragma region Utility Methods

void URigidBodyComponent::OnWorldTransformChanged(const FTransform& NewTransform)
{
    // 더티 플래그 설정
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));
}

void URigidBodyComponent::MarkDataDirty(const FPhysicsDataDirtyFlags& flags)
{
    DirtyFlags |= flags;
}

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

void URigidBodyComponent::InitializeTimeInterpolation()
{
    bEnableTimeInterpolation = true;
    InterpolationAlpha = 0.0f;
    LastTickTime = 0.0f;
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
        SetWorldTransform(FTransform(PhysicsResults.ResultPosition,
                                     PhysicsResults.ResultRotation,
                                     PhysicsResults.ResultScale));
        return;
    }

    // SIMD를 위한 XMVECTOR 로드
    XMVECTOR CurrentPos = XMLoadFloat3(&CurrentGameTransform.Position);
    XMVECTOR PreviousPos = XMLoadFloat3(&PreviousPosition);
    XMVECTOR PhysicsResultPos = XMLoadFloat3(&PhysicsResults.ResultPosition);

    XMVECTOR CurrentRot = XMLoadFloat4(&CurrentGameTransform.Rotation);
    XMVECTOR PreviousRot = XMLoadFloat4(&PreviousRotation);
    XMVECTOR PhysicsResultRot = XMLoadFloat4(&PhysicsResults.ResultRotation);

    float PhysicsWeight = 0.7f;
    XMVECTOR PhysicsWeightVec = XMVectorSet(PhysicsWeight, PhysicsWeight, PhysicsWeight, PhysicsWeight);

    // Vector3 변화량 분리 (SIMD 연산)
    XMVECTOR GameLogicDelta = XMVectorSubtract(CurrentPos, PreviousPos);
    XMVECTOR PhysicsDelta = XMVectorSubtract(PhysicsResultPos, PreviousPos);

    // 최종 Position 계산 (SIMD 연산)
    XMVECTOR PhysicsDeltaWeighted = XMVectorMultiply(PhysicsDelta, PhysicsWeightVec);
    XMVECTOR FinalPositionVec = XMVectorAdd(PreviousPos, GameLogicDelta);
    FinalPositionVec = XMVectorAdd(FinalPositionVec, PhysicsDeltaWeighted);

    // 회전 보간 (SIMD 연산)
    XMVECTOR FinalRotationVec = XMQuaternionSlerp(CurrentRot, PhysicsResultRot, PhysicsWeight);

    // 스케일은 게임 로직 우선 (SIMD로 로드해서 반환)
    XMVECTOR FinalScaleVec = XMLoadFloat3(&CurrentGameTransform.Scale);

    // 최종 FTransform 생성 (Store 연산)
    FTransform FinalTransform;
    XMStoreFloat3(&FinalTransform.Position, FinalPositionVec);
    XMStoreFloat4(&FinalTransform.Rotation, FinalRotationVec);
    XMStoreFloat3(&FinalTransform.Scale, FinalScaleVec);

    SetWorldTransform(FinalTransform);
    // 디버그 로그 (임시)
    float PositionDifference = (CurrentGameTransform.Position - PhysicsResults.ResultPosition).Length();
    if (PositionDifference > 1.0f)
    {
        LOG_INFO("Delta-based Transform: Game [%s] \n Physics[%s] \n Final[%s]",
                 Debug::ToString(CurrentGameTransform.Position),
                 Debug::ToString(CurrentGameTransform.Position),
                 Debug::ToString(FinalTransform.Position));
    }
}
#pragma endregion




