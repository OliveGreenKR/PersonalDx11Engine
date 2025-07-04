#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "Debug.h"
#include "PhysicsDefine.h"
#include "PhysicsSystem.h"
#include "PhysicsJob.h"

#pragma region Constructor and Lifecycle

URigidBodyComponent::URigidBodyComponent()
{
    bPhysicsSimulated = true;
    InitializeGameState();
    InitializePhysicsCache();
}

URigidBodyComponent::~URigidBodyComponent()
{
    // 물리 시스템에서 안전하게 해제
    if (bIsRegisteredToPhysicsSystem)
    {
        UnRegisterPhysicsSystem();
    }
}

void URigidBodyComponent::PostInitialized()
{
    USceneComponent::PostInitialized();

    // 현재 SceneComponent Transform을 게임 상태로 설정
    FTransform currentTransform = USceneComponent::GetWorldTransform();
    HighFrequencyGameState = FHighFrequencyData(currentTransform);
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));
}

void URigidBodyComponent::PostTreeInitialized()
{
    USceneComponent::PostTreeInitialized();

    // 물리 시스템 등록 (아직 등록되지 않은 경우)
    if (!bIsRegisteredToPhysicsSystem)
    {
        RegisterPhysicsSystem();
    }
}

void URigidBodyComponent::Activate()
{
    USceneComponent::Activate();

    // 물리 시스템 등록 (아직 등록되지 않은 경우)
    if (!bIsRegisteredToPhysicsSystem)
    {
        RegisterPhysicsSystem();
    }

    // 물리 활성화 상태 업데이트
    SetPhysicsActive(true);
}

void URigidBodyComponent::DeActivate()
{
    USceneComponent::DeActivate();

    // 물리 비활성화 상태 업데이트
    SetPhysicsActive(false);
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
    USceneComponent::Tick(DeltaTime);

    if (!IsActive())
        return;
}

#pragma endregion

#pragma region IPhysicsObject Implementation

FHighFrequencyData URigidBodyComponent::GetHighFrequencyData() const
{
    return HighFrequencyGameState;
}

FMidFrequencyData URigidBodyComponent::GetMidFrequencyData() const
{
    return MidFrequencyGameState;
}

FLowFrequencyData URigidBodyComponent::GetLowFrequencyData() const
{
    return LowFrequencyGameState;
}

void URigidBodyComponent::ReceivePhysicsResults(const FPhysicsToGameData& results)
{
    // 물리 결과를 캐시에 저장
    PhysicsResultCache = results;

    // SceneComponent Transform 업데이트 (계층구조 동기화)
    FTransform resultTransform = results.GetResultTransform();
    USceneComponent::SetWorldTransform(resultTransform);

    // 게임 상태도 물리 결과로 업데이트 (일관성 유지)
    HighFrequencyGameState = FHighFrequencyData(resultTransform);
}

FPhysicsDataDirtyFlags URigidBodyComponent::GetDirtyFlags() const
{
    return DirtyFlags;
}

void URigidBodyComponent::MarkDataClean(const FPhysicsDataDirtyFlags& flags)
{
    DirtyFlags.ClearFlag(flags.GetRawFlags());
}

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

    // IPhysicsObject로 등록
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

void URigidBodyComponent::TickPhysics(const float DeltaTime)
{
    // 게임플레이 로직과 물리 시스템 간 상호작용 처리
    // 직접적인 물리 계산은 PhysicsSystem에서 배치 처리됨
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

#pragma region Game Logic Interface (Immediate Updates)

void URigidBodyComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
    // 게임 상태 즉시 업데이트
    HighFrequencyGameState = FHighFrequencyData(InWorldTransform);
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));

    // SceneComponent 계층구조도 즉시 업데이트
    USceneComponent::SetWorldTransform(InWorldTransform);
}

void URigidBodyComponent::SetWorldPosition(const Vector3& InPosition)
{
    HighFrequencyGameState.Position = InPosition;
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));

    // SceneComponent 동기화
    FTransform newTransform = HighFrequencyGameState.GetTransform();
    USceneComponent::SetWorldTransform(newTransform);
}

void URigidBodyComponent::SetWorldRotation(const Quaternion& InRotation)
{
    HighFrequencyGameState.Rotation = InRotation;
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));

    // SceneComponent 동기화
    FTransform newTransform = HighFrequencyGameState.GetTransform();
    USceneComponent::SetWorldTransform(newTransform);
}

void URigidBodyComponent::SetWorldScale(const Vector3& InScale)
{
    HighFrequencyGameState.Scale = InScale;
    MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_HIGH_FREQ));

    // SceneComponent 동기화
    FTransform newTransform = HighFrequencyGameState.GetTransform();
    USceneComponent::SetWorldTransform(newTransform);
}

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
    FPhysicsMask currentMask = MidFrequencyGameState.PhysicsMask;

    if (bEnabled)
    {
        currentMask.SetFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    }
    else
    {
        currentMask.ClearFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    }

    if (currentMask != MidFrequencyGameState.PhysicsMask)
    {
        MidFrequencyGameState.PhysicsMask = currentMask;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_MID_FREQ));
    }
}

void URigidBodyComponent::SetPhysicsActive(bool bActive)
{
    FPhysicsMask currentMask = MidFrequencyGameState.PhysicsMask;

    if (bActive)
    {
        currentMask.SetFlag(FPhysicsMask::MASK_ACTIVATION);
    }
    else
    {
        currentMask.ClearFlag(FPhysicsMask::MASK_ACTIVATION);
    }

    if (currentMask != MidFrequencyGameState.PhysicsMask)
    {
        MidFrequencyGameState.PhysicsMask = currentMask;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_MID_FREQ));
    }
}

void URigidBodyComponent::SetMass(float InMass)
{
    if (InMass <= KINDA_SMALL)
    {
        LOG_WARNING("Invalid mass value: %f", InMass);
        return;
    }

    float newInvMass = 1.0f / InMass;
    if (abs(LowFrequencyGameState.InvMass - newInvMass) > KINDA_SMALL)
    {
        LowFrequencyGameState.InvMass = newInvMass;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetFrictionKinetic(float InFriction)
{
    float clampedFriction = Math::Max(InFriction, 0.0f);
    if (abs(LowFrequencyGameState.FrictionKinetic - clampedFriction) > KINDA_SMALL)
    {
        LowFrequencyGameState.FrictionKinetic = clampedFriction;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetFrictionStatic(float InFriction)
{
    float clampedFriction = Math::Max(InFriction, 0.0f);
    if (abs(LowFrequencyGameState.FrictionStatic - clampedFriction) > KINDA_SMALL)
    {
        LowFrequencyGameState.FrictionStatic = clampedFriction;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetRestitution(float InRestitution)
{
    float clampedRestitution = Math::Clamp(InRestitution, 0.0f, 1.0f);
    if (abs(LowFrequencyGameState.Restitution - clampedRestitution) > KINDA_SMALL)
    {
        LowFrequencyGameState.Restitution = clampedRestitution;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetInvRotationalInertia(const Vector3& InValue)
{
    if ((LowFrequencyGameState.InvRotationalInertia - InValue).LengthSquared() > KINDA_SMALL * KINDA_SMALL)
    {
        LowFrequencyGameState.InvRotationalInertia = InValue;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetMaxSpeed(float InSpeed)
{
    if (abs(LowFrequencyGameState.MaxSpeed - InSpeed) > KINDA_SMALL)
    {
        LowFrequencyGameState.MaxSpeed = InSpeed;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetMaxAngularSpeed(float InSpeed)
{
    if (abs(LowFrequencyGameState.MaxAngularSpeed - InSpeed) > KINDA_SMALL)
    {
        LowFrequencyGameState.MaxAngularSpeed = InSpeed;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

void URigidBodyComponent::SetGravityScale(float InScale)
{
    if (abs(LowFrequencyGameState.GravityScale - InScale) > KINDA_SMALL)
    {
        LowFrequencyGameState.GravityScale = InScale;
        MarkDataDirty(FPhysicsDataDirtyFlags(FPhysicsDataDirtyFlags::FLAG_LOW_FREQ));
    }
}

#pragma endregion

#pragma region Physics State Queries (Cached Values)

Vector3 URigidBodyComponent::GetVelocity() const
{
    return PhysicsResultCache.Velocity;
}

Vector3 URigidBodyComponent::GetAngularVelocity() const
{
    return PhysicsResultCache.AngularVelocity;
}

float URigidBodyComponent::GetMass() const
{
    float invMass = GetInvMass();
    return invMass > KINDA_SMALL ? (1.0f / invMass) : KINDA_LARGE;
}

float URigidBodyComponent::GetInvMass() const
{
    return IsStatic() ? 0.0f : LowFrequencyGameState.InvMass;
}

Vector3 URigidBodyComponent::GetRotationalInertia() const
{
    Vector3 invInertia = LowFrequencyGameState.InvRotationalInertia;
    Vector3 result;

    result.x = (abs(invInertia.x) > KINDA_SMALL) ? (1.0f / invInertia.x) : KINDA_LARGE;
    result.y = (abs(invInertia.y) > KINDA_SMALL) ? (1.0f / invInertia.y) : KINDA_LARGE;
    result.z = (abs(invInertia.z) > KINDA_SMALL) ? (1.0f / invInertia.z) : KINDA_LARGE;

    return result;
}

Vector3 URigidBodyComponent::GetInvRotationalInertia() const
{
    return LowFrequencyGameState.InvRotationalInertia;
}

float URigidBodyComponent::GetRestitution() const
{
    return LowFrequencyGameState.Restitution;
}

float URigidBodyComponent::GetFrictionKinetic() const
{
    return LowFrequencyGameState.FrictionKinetic;
}

float URigidBodyComponent::GetFrictionStatic() const
{
    return LowFrequencyGameState.FrictionStatic;
}

float URigidBodyComponent::GetSpeed() const
{
    return PhysicsResultCache.Velocity.Length();
}

bool URigidBodyComponent::IsGravityEnabled() const
{
    return MidFrequencyGameState.PhysicsMask.HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
}

bool URigidBodyComponent::IsPhysicsActive() const
{
    return MidFrequencyGameState.PhysicsMask.HasFlag(FPhysicsMask::MASK_ACTIVATION);
}

bool URigidBodyComponent::IsStatic() const
{
    return MidFrequencyGameState.PhysicsType == EPhysicsType::Static;
}

bool URigidBodyComponent::IsDynamic() const
{
    return MidFrequencyGameState.PhysicsType == EPhysicsType::Dynamic;
}

EPhysicsType URigidBodyComponent::GetPhysicsType() const
{
    return MidFrequencyGameState.PhysicsType;
}

#pragma endregion

#pragma region Job-Based Physics Commands (Immediate Actions)

void URigidBodyComponent::ApplyForce(const Vector3& Force)
{
    ApplyForce(Force, GetCenterOfMass());
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
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
        PhysicsSystem->RequestPhysicsJob<FJobApplyForce>(PhysicsObjectID, Force, Location);
    }
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse)
{
    ApplyImpulse(Impulse, GetCenterOfMass());
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
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
        PhysicsSystem->RequestPhysicsJob<FJobApplyImpulse>(PhysicsObjectID, Impulse, Location);
    }
}

void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetVelocity - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetVelocity>(PhysicsObjectID, InVelocity);
    }
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::AddVelocity - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobAddVelocity>(PhysicsObjectID, InVelocityDelta);
    }
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetAngularVelocity - Component not registered to physics system");
        return;
    }

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

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobAddAngularVelocity>(PhysicsObjectID, InAngularVelocityDelta);
    }
}

#pragma endregion

#pragma region Internal Helpers

void URigidBodyComponent::MarkDataDirty(const FPhysicsDataDirtyFlags& flags)
{
    DirtyFlags |= flags;
}

void URigidBodyComponent::SetPhysicsID(PhysicsID InID)
{
    PhysicsObjectID = InID;
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

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
    // 현재는 Transform의 Position을 질량 중심으로 사용
    // 향후 복잡한 형태의 경우 별도 계산 가능
    return HighFrequencyGameState.Position;
}
#pragma endregion