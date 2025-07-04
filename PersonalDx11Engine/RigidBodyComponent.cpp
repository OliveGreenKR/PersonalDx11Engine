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

    // 캐시는 나중에 물리 시스템에서 동기화될 예정 (직접 설정하지 않음)
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

void URigidBodyComponent::ResetPhysicsState()
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::ResetPhysicsState - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
    {
        LOG_ERROR("URigidBodyComponent::ResetPhysicsState - PhysicsSystem not available");
        return;
    }

    // FPhysicsState 기본값으로 모든 물리 상태를 Job을 통해 순차 리셋
    FPhysicsState DefaultState;

    // 속도 리셋
    PhysicsSystem->RequestPhysicsJob<FJobSetVelocity>(PhysicsObjectID, DefaultState.Velocity);
    PhysicsSystem->RequestPhysicsJob<FJobSetAngularVelocity>(PhysicsObjectID, DefaultState.AngularVelocity);

    // 물리 속성 리셋
    PhysicsSystem->RequestPhysicsJob<FJobSetInvMass>(PhysicsObjectID, DefaultState.InvMass);
    PhysicsSystem->RequestPhysicsJob<FJobSetInvRotationalInertia>(PhysicsObjectID, DefaultState.InvRotationalInertia);
    PhysicsSystem->RequestPhysicsJob<FJobSetFrictionKinetic>(PhysicsObjectID, DefaultState.FrictionKinetic);
    PhysicsSystem->RequestPhysicsJob<FJobSetFrictionStatic>(PhysicsObjectID, DefaultState.FrictionStatic);
    PhysicsSystem->RequestPhysicsJob<FJobSetRestitution>(PhysicsObjectID, DefaultState.Restitution);
    PhysicsSystem->RequestPhysicsJob<FJobSetMaxSpeed>(PhysicsObjectID, DefaultState.MaxSpeed);
    PhysicsSystem->RequestPhysicsJob<FJobSetMaxAngularSpeed>(PhysicsObjectID, DefaultState.MaxAngularSpeed);

    // 타입 및 마스크 리셋
    PhysicsSystem->RequestPhysicsJob<FJobSetPhysicsType>(PhysicsObjectID, DefaultState.PhysicsType);
    PhysicsSystem->RequestPhysicsJob<FJobSetPhysicsMask>(PhysicsObjectID, DefaultState.PhysicsMasks);

    // 캐시는 Sync 시점에 물리 시스템에서 업데이트됨
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

    // Job으로만 물리 시스템에 전달 (실제 SceneComponent Transform 업데이트는 Sync 시점에)
    if (bIsRegisteredToPhysicsSystem && PhysicsObjectID != 0)
    {
        UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
        if (PhysicsSystem)
        {
            PhysicsSystem->RequestPhysicsJob<FJobSetWorldTransform>(PhysicsObjectID, InWorldTransform);
        }
    }

    // 캐시는 물리 시스템에서만 업데이트 (여기서는 수정하지 않음)
}
#pragma endregion

#pragma region IPhysicsState Implementation (Job-Based)
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
#pragma endregion

#pragma region IPhysicsObject Implementation (Latest Interface)
void URigidBodyComponent::SynchronizeCachedStateFromSimulated()
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
        return;

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
        return;

    // PhysicsSystem에서 최신 상태를 가져와 캐시 업데이트 (유일한 캐시 Write 지점)
    CachedPhysicsState.Velocity = PhysicsSystem->P_GetVelocity(PhysicsObjectID);
    CachedPhysicsState.AngularVelocity = PhysicsSystem->P_GetAngularVelocity(PhysicsObjectID);
    CachedPhysicsState.WorldTransform = PhysicsSystem->P_GetWorldTransform(PhysicsObjectID);
    CachedPhysicsState.PhysicsMasks = PhysicsSystem->P_GetPhysicsMask(PhysicsObjectID);
    CachedPhysicsState.PhysicsType = PhysicsSystem->P_GetPhysicsType(PhysicsObjectID);
    CachedPhysicsState.InvMass = PhysicsSystem->P_GetInvMass(PhysicsObjectID);
    CachedPhysicsState.InvRotationalInertia = PhysicsSystem->P_GetInvRotationalInertia(PhysicsObjectID);
    CachedPhysicsState.FrictionKinetic = PhysicsSystem->P_GetFrictionKinetic(PhysicsObjectID);
    CachedPhysicsState.FrictionStatic = PhysicsSystem->P_GetFrictionStatic(PhysicsObjectID);
    CachedPhysicsState.Restitution = PhysicsSystem->P_GetRestitution(PhysicsObjectID);
    CachedPhysicsState.MaxSpeed = PhysicsSystem->P_GetMaxSpeed(PhysicsObjectID);
    CachedPhysicsState.MaxAngularSpeed = PhysicsSystem->P_GetMaxAngularSpeed(PhysicsObjectID);

    // 실제 SceneComponent의 Transform 계층구조 업데이트는 이 시점에!
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

    // IPhysicsObject로 등록 (물리 시스템이 FPhysicsState 기본값으로 자동 초기화)
    std::shared_ptr<IPhysicsObject> PhysicsObjectPtr = Engine::Cast<IPhysicsObject>(
        Engine::Cast<URigidBodyComponent>(shared_from_this()));
    PhysicsObjectID = PhysicsSystem->RegisterPhysicsObject(PhysicsObjectPtr);

    if (PhysicsObjectID != 0)
    {
        bIsRegisteredToPhysicsSystem = true;

        // 현재 Transform만 물리 시스템에 설정 (위치 동기화용)
        PhysicsSystem->RequestPhysicsJob<FJobSetWorldTransform>(PhysicsObjectID, GetWorldTransform());

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
    // 더 이상 직접적인 물리 계산을 수행하지 않음
    // PhysicsSystem에서 배치 처리로 모든 물리 연산 수행
    // 필요시 여기서 게임플레이 로직 관련 물리 처리 수행 가능
}
#pragma endregion

#pragma region Physics Property Settings (Job-Based)
void URigidBodyComponent::SetMass(float InMass)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetMass - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetMass>(PhysicsObjectID, InMass);
        // 캐시는 물리 시스템에서 업데이트됨 (여기서 수정하지 않음)
    }
}

void URigidBodyComponent::SetFrictionKinetic(float InFriction)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetFrictionKinetic - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetFrictionKinetic>(PhysicsObjectID, InFriction);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetFrictionStatic(float InFriction)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetFrictionStatic - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetFrictionStatic>(PhysicsObjectID, InFriction);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetRestitution(float InRestitution)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetRestitution - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetRestitution>(PhysicsObjectID, InRestitution);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetInvRotationalInertia(const Vector3& Value)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetInvRotationalInertia - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetInvRotationalInertia>(PhysicsObjectID, Value);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetMaxSpeed(float InSpeed)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetMaxSpeed - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetMaxSpeed>(PhysicsObjectID, InSpeed);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetMaxAngularSpeed(float InSpeed)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetMaxAngularSpeed - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetMaxAngularSpeed>(PhysicsObjectID, InSpeed);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetGravityScale(float InScale)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetGravityScale - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetGravityScale>(PhysicsObjectID, InScale);
        // GravityScale은 캐시에 저장되지 않음
    }
}

void URigidBodyComponent::SetPhysicsType(EPhysicsType InType)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetPhysicsType - Component not registered to physics system");
        return;
    }

    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (PhysicsSystem)
    {
        PhysicsSystem->RequestPhysicsJob<FJobSetPhysicsType>(PhysicsObjectID, InType);
        // 캐시는 물리 시스템에서 업데이트됨
    }
}

void URigidBodyComponent::SetGravityEnabled(bool bEnabled)
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
    {
        LOG_WARNING("URigidBodyComponent::SetGravityEnabled - Component not registered to physics system");
        return;
    }

    // 물리 시스템에서 현재 마스크를 가져와서 중력 플래그만 업데이트
    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
        return;

    FPhysicsMask CurrentMask = PhysicsSystem->P_GetPhysicsMask(PhysicsObjectID);

    if (bEnabled)
    {
        CurrentMask.SetFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    }
    else
    {
        CurrentMask.ClearFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED);
    }

    PhysicsSystem->RequestPhysicsJob<FJobSetPhysicsMask>(PhysicsObjectID, CurrentMask);
}
#pragma endregion

#pragma region Private Helpers and Internal Methods
Vector3 URigidBodyComponent::GetCenterOfMass() const
{
    return CachedPhysicsState.WorldTransform.Position;
}

void URigidBodyComponent::UpdatePhysicsActivationState()
{
    if (!bIsRegisteredToPhysicsSystem || PhysicsObjectID == 0)
        return;

    // 물리 시스템에서 현재 마스크를 가져와서 활성화 상태만 업데이트
    UPhysicsSystem* PhysicsSystem = UPhysicsSystem::Get();
    if (!PhysicsSystem)
        return;

    FPhysicsMask CurrentMask = PhysicsSystem->P_GetPhysicsMask(PhysicsObjectID);

    // ActorComponent 활성화 상태와 동기화
    if (IsActive())
    {
        CurrentMask.SetFlag(FPhysicsMask::MASK_ACTIVATION);
    }
    else
    {
        CurrentMask.ClearFlag(FPhysicsMask::MASK_ACTIVATION);
    }

    PhysicsSystem->RequestPhysicsJob<FJobSetPhysicsMask>(PhysicsObjectID, CurrentMask);
}
#pragma endregion