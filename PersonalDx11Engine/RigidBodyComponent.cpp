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

    // 초기 물리 상태 설정
    CachedPhysicsState.WorldTransform = GetWorldTransform();
    CachedPhysicsState.PhysicsType = EPhysicsType::Dynamic;
    CachedPhysicsState.PhysicsMasks = FPhysicsMask(FPhysicsMask::GROUP_BASIC_SIMULATION);

    // 기본 물리 속성 설정
    CachedPhysicsState.InvMass = 1.0f;
    CachedPhysicsState.InvRotationalInertia = Vector3(1.0f, 1.0f, 1.0f);
    CachedPhysicsState.FrictionKinetic = 0.3f;
    CachedPhysicsState.FrictionStatic = 0.5f;
    CachedPhysicsState.Restitution = 0.2f;
    CachedPhysicsState.MaxSpeed = 3.0f * ONE_METER;
    CachedPhysicsState.MaxAngularSpeed = PI * 2.0f;
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
    UpdatePhysicsActivationState();
}

void URigidBodyComponent::DeActivate()
{
    USceneComponent::DeActivate();

    // 물리 비활성화 상태 업데이트
    UpdatePhysicsActivationState();
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
    USceneComponent::Tick(DeltaTime);

    if (!IsActive())
        return;
}
#pragma endregion

#pragma region SceneComponent Override
void URigidBodyComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
    USceneComponent::SetWorldTransform(InWorldTransform);

    // 트랜스폼 변경을 물리 시스템에 Job으로 전달
    if (bIsRegisteredToPhysicsSystem && PhysicsID != 0)
    {
        RequestPhysicsJob<FJobSetWorldTransform>(InWorldTransform);
    }

    // 캐시 업데이트
    CachedPhysicsState.WorldTransform = InWorldTransform;
}
#pragma endregion

#pragma region IPhysicsState Implementation (Job-Based)
void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetVelocity - Component not registered to physics system");
        return;
    }

    RequestPhysicsJob<FJobSetVelocity>(InVelocity);
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        LOG_WARNING("URigidBodyComponent::AddVelocity - Component not registered to physics system");
        return;
    }

    RequestPhysicsJob<FJobAddVelocity>(InVelocityDelta);
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetAngularVelocity - Component not registered to physics system");
        return;
    }

    RequestPhysicsJob<FJobSetAngularVelocity>(InAngularVelocity);
}

void URigidBodyComponent::AddAngularVelocity(const Vector3& InAngularVelocityDelta)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        LOG_WARNING("URigidBodyComponent::AddAngularVelocity - Component not registered to physics system");
        return;
    }

    RequestPhysicsJob<FJobAddAngularVelocity>(InAngularVelocityDelta);
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        LOG_WARNING("URigidBodyComponent::ApplyForce - Component not registered to physics system");
        return;
    }

    if (!IsActive() || IsStatic())
        return;

    RequestPhysicsJob<FJobApplyForce>(Force, Location);
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        LOG_WARNING("URigidBodyComponent::ApplyImpulse - Component not registered to physics system");
        return;
    }

    if (!IsActive() || IsStatic())
        return;

    RequestPhysicsJob<FJobApplyImpulse>(Impulse, Location);
}
#pragma endregion

#pragma region IPhysicsObject Implementation (Latest Interface)
void URigidBodyComponent::SynchronizeCachedStateFromSimulated()
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
        return;

    // PhysicsSystem에서 최신 상태를 가져와 캐시 업데이트
    CachedPhysicsState.Velocity = PhysicsSystem->P_GetVelocity(PhysicsID);
    CachedPhysicsState.AngularVelocity = PhysicsSystem->P_GetAngularVelocity(PhysicsID);
    CachedPhysicsState.WorldTransform = PhysicsSystem->P_GetWorldTransform(PhysicsID);
    CachedPhysicsState.PhysicsMasks = PhysicsSystem->P_GetPhysicsMask(PhysicsID);
    CachedPhysicsState.PhysicsType = PhysicsSystem->P_GetPhysicsType(PhysicsID);

    // SceneComponent의 Transform도 동기화
    USceneComponent::SetWorldTransform(CachedPhysicsState.WorldTransform);
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
    std::shared_ptr<IPhysicsObject> PhysicsObjectPtr = std::static_pointer_cast<IPhysicsObject>(shared_from_this());
    PhysicsID = PhysicsSystem->RegisterPhysicsObject(PhysicsObjectPtr);

    if (PhysicsID != 0)
    {
        bIsRegisteredToPhysicsSystem = true;

        // 현재 캐시된 상태를 물리 시스템에 설정
        InitializePhysicsSystemState();

        LOG_INFO("URigidBodyComponent registered to PhysicsSystem with ID: %u", PhysicsID);
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
    if (PhysicsSystem && PhysicsID != 0)
    {
        PhysicsSystem->UnregisterPhysicsObject(PhysicsID);
        LOG_INFO("URigidBodyComponent unregistered from PhysicsSystem with ID: %u", PhysicsID);
    }

    PhysicsID = 0;
    bIsRegisteredToPhysicsSystem = false;
}

void URigidBodyComponent::TickPhysics(const float DeltaTime)
{
    // 더 이상 직접적인 물리 계산을 수행하지 않음
    // PhysicsSystem에서 배치 처리로 모든 물리 연산 수행
    // 필요시 여기서 게임플레이 로직 관련 물리 처리 수행 가능
}
#pragma endregion

#pragma region Physics Property Settings (Job-Based)
void URigidBodyComponent::SetMass(float InMass)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        // 캐시에만 저장 (등록 시 물리 시스템에 전달됨)
        CachedPhysicsState.InvMass = InMass > KINDA_LARGE ? 0.0f : 1.0f / InMass;
        return;
    }

    float InvMass = InMass > KINDA_LARGE ? 0.0f : 1.0f / InMass;
    UPhysicsSystem::Get()->P_SetInvMass(PhysicsID, InvMass);

    // 캐시 업데이트
    CachedPhysicsState.InvMass = InvMass;
}

void URigidBodyComponent::SetFrictionKinetic(float InFriction)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.FrictionKinetic = InFriction;
        return;
    }

    UPhysicsSystem::Get()->P_SetFrictionKinetic(PhysicsID, InFriction);
    CachedPhysicsState.FrictionKinetic = InFriction;
}

void URigidBodyComponent::SetFrictionStatic(float InFriction)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.FrictionStatic = InFriction;
        return;
    }

    UPhysicsSystem::Get()->P_SetFrictionStatic(PhysicsID, InFriction);
    CachedPhysicsState.FrictionStatic = InFriction;
}

void URigidBodyComponent::SetRestitution(float InRestitution)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.Restitution = InRestitution;
        return;
    }

    UPhysicsSystem::Get()->P_SetRestitution(PhysicsID, InRestitution);
    CachedPhysicsState.Restitution = InRestitution;
}

void URigidBodyComponent::SetInvRotationalInertia(const Vector3& Value)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.InvRotationalInertia = Value;
        return;
    }

    UPhysicsSystem::Get()->P_SetInvRotationalInertia(PhysicsID, Value);
    CachedPhysicsState.InvRotationalInertia = Value;
}

void URigidBodyComponent::SetMaxSpeed(float InSpeed)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.MaxSpeed = InSpeed;
        return;
    }

    UPhysicsSystem::Get()->P_SetMaxSpeed(PhysicsID, InSpeed);
    CachedPhysicsState.MaxSpeed = InSpeed;
}

void URigidBodyComponent::SetMaxAngularSpeed(float InSpeed)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.MaxAngularSpeed = InSpeed;
        return;
    }

    UPhysicsSystem::Get()->P_SetMaxAngularSpeed(PhysicsID, InSpeed);
    CachedPhysicsState.MaxAngularSpeed = InSpeed;
}

void URigidBodyComponent::SetGravityScale(float InScale)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        // 캐시에는 직접 저장할 수 없으므로 등록 후 설정 필요
        return;
    }

    UPhysicsSystem::Get()->P_SetGravityScale(PhysicsID, InScale);
}

void URigidBodyComponent::SetPhysicsType(EPhysicsType InType)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.PhysicsType = InType;
        return;
    }

    UPhysicsSystem::Get()->P_SetPhysicsType(PhysicsID, InType);
    CachedPhysicsState.PhysicsType = InType;
}

void URigidBodyComponent::SetGravityEnabled(bool bEnabled)
{
    FPhysicsMask NewMask = CachedPhysicsState.PhysicsMasks;

    if (bEnabled)
    {
        NewMask.SetFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    }
    else
    {
        NewMask.ClearFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    }

    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.PhysicsMasks = NewMask;
        return;
    }

    UPhysicsSystem::Get()->P_SetPhysicsMask(PhysicsID, NewMask);
    CachedPhysicsState.PhysicsMasks = NewMask;
}
#pragma endregion

#pragma region Private Helpers and Internal Methods
Vector3 URigidBodyComponent::GetCenterOfMass() const
{
    return GetWorldTransform().Position;
}

template<typename JobType, typename... Args>
void URigidBodyComponent::RequestPhysicsJob(Args&&... args)
{
    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
    {
        LOG_ERROR("URigidBodyComponent::RequestPhysicsJob - PhysicsSystem not available");
        return;
    }

    PhysicsSystem->RequestPhysicsJob<JobType>(PhysicsID, std::forward<Args>(args)...);
}

void URigidBodyComponent::UpdatePhysicsActivationState()
{
    FPhysicsMask NewMask = CachedPhysicsState.PhysicsMasks;

    // ActorComponent 활성화 상태와 동기화
    if (IsActive())
    {
        NewMask.SetFlag(FPhysicsMask::MASK_ACTIVATION);
    }
    else
    {
        NewMask.ClearFlag(FPhysicsMask::MASK_ACTIVATION);
    }

    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
    {
        CachedPhysicsState.PhysicsMasks = NewMask;
        return;
    }

    UPhysicsSystem::Get()->P_SetPhysicsMask(PhysicsID, NewMask);
    CachedPhysicsState.PhysicsMasks = NewMask;
}

void URigidBodyComponent::InitializePhysicsSystemState()
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsID == 0)
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
        return;

    // 캐시된 모든 상태를 물리 시스템에 설정
    PhysicsSystem->P_SetWorldTransform(PhysicsID, CachedPhysicsState.WorldTransform);
    PhysicsSystem->P_SetInvMass(PhysicsID, CachedPhysicsState.InvMass);
    PhysicsSystem->P_SetInvRotationalInertia(PhysicsID, CachedPhysicsState.InvRotationalInertia);
    PhysicsSystem->P_SetFrictionKinetic(PhysicsID, CachedPhysicsState.FrictionKinetic);
    PhysicsSystem->P_SetFrictionStatic(PhysicsID, CachedPhysicsState.FrictionStatic);
    PhysicsSystem->P_SetRestitution(PhysicsID, CachedPhysicsState.Restitution);
    PhysicsSystem->P_SetMaxSpeed(PhysicsID, CachedPhysicsState.MaxSpeed);
    PhysicsSystem->P_SetMaxAngularSpeed(PhysicsID, CachedPhysicsState.MaxAngularSpeed);
    PhysicsSystem->P_SetPhysicsType(PhysicsID, CachedPhysicsState.PhysicsType);
    PhysicsSystem->P_SetPhysicsMask(PhysicsID, CachedPhysicsState.PhysicsMasks);

    LOG_INFO("URigidBodyComponent::InitializePhysicsSystemState - State synchronized to PhysicsSystem");
}
#pragma endregion