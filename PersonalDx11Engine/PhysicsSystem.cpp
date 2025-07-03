#include "PhysicsSystem.h"
#include "CollisionProcessor.h"
#include <algorithm>
#include "Debug.h"
#include "ConfigReadManager.h"

#pragma region Constructors and PhysicsObjects LifeCycle Management
UPhysicsSystem::UPhysicsSystem()
    : PhysicsStateSoA(128)
    , JobPool(sizeof(FPhysicsJob) * 1024)  // 기본 1024개 Job 크기로 초기화
    , JobQueue(512)  // 기본 512개 큐 크기
{
}

UPhysicsSystem::~UPhysicsSystem()
{
    Release();
}

void UPhysicsSystem::Initialize()
{
    try
    {
        LoadConfigFromIni();
        PhysicsStateSoA.TryResize(InitialPhysicsObjectCapacity);
        JobPool.Initialize(InitialPhysicsJobPoolSize * sizeof(FPhysicsJob));
    }
    catch (...)
    {
        LOG_ERROR("PhysicsSystem Intialization Failed");
        Release();
        exit(1);
    }

    LOG_INFO("PhysicsSystem Intialized successfully.");
    return;
}

void UPhysicsSystem::Release()
{
    //큐 정리
    JobQueue.Clear();
    //풀 정리
    JobPool.Reset();
    
    //PhysicsStateSoA는 자동정리
}

void UPhysicsSystem::LoadConfigFromIni()
{
    UConfigReadManager::Get()->GetValue("InitialPhysicsObjectCapacity", InitialPhysicsObjectCapacity);
    UConfigReadManager::Get()->GetValue("InitialPhysicsJobPoolSize", InitialPhysicsJobPoolSize);
    UConfigReadManager::Get()->GetValue("FixedTimeStep", FixedTimeStep);
    UConfigReadManager::Get()->GetValue("MinSubStepTickTime", MinSubStepTickTime);
    UConfigReadManager::Get()->GetValue("MaxSubSteps", MaxSubSteps);
    UConfigReadManager::Get()->GetValue("MinSubSteps", MinSubSteps);
}


SoAID UPhysicsSystem::RegisterPhysicsObject(std::shared_ptr<IPhysicsObject>& Object)
{
    if (!Object)
    {
        LOG_WARNING("Invalid PhysicsObject try to Register to PhysiscSystem.");
        return FPhysicsStateArrays::INVALID_ID;
    }

    std::weak_ptr<IPhysicsObject> weakRef = Object;
    SoAID newID = PhysicsStateSoA.AllocateSlot(weakRef);

    if (newID != FPhysicsStateArrays::INVALID_ID)
    {
        LOG_INFO("Physics Object Registered - ID: {%u}", newID);
    }
    else
    {
        LOG_ERROR("Failed to register physics object - SoA full");
    }

    return newID;
}

void UPhysicsSystem::UnregisterPhysicsObject(SoAID id)
{
    if (IsValidTargetID(id))
    {
        PhysicsStateSoA.DeallocateSlot(id);
        LOG_INFO("Physics Object Unregistered - ID: {}", id);
    }
    else
    {
        LOG_ERROR("Invalid SoAID for unregistration: {}", id);
    }
}

// 메인 물리 업데이트 (메인 루프에서 호출)
void UPhysicsSystem::TickPhysics(const float DeltaTime)
{
    // 시간 누적 및 서브스텝 계산
    AccumulatedTime += DeltaTime;
    int NumSubsteps = CalculateRequiredSubsteps();
    NumSubsteps = Math::Clamp(NumSubsteps, 0, MaxSubSteps);

    if (NumSubsteps < 1)
        return;

    // 물리 시뮬레이션 준비
    PrepareSimulation();

    float TimeStep = FixedTimeStep;
    // 서브스텝 시뮬레이션
    for (int i = 0; i < NumSubsteps; i++)
    {
        float SimualtedTime = SimulateSubstep(TimeStep);
		TimeStep -= SimualtedTime;

        // 시간 전부 사용- 서브  스텝 종료
        if (TimeStep < KINDA_SMALL && i > MinSubSteps)
        {
            break;
        }
        AccumulatedTime -= SimualtedTime;

    }

    // 시뮬레이션 결과 적용
    FinalizeSimulation();
}


// 필요한 서브스텝 수 계산
int UPhysicsSystem::CalculateRequiredSubsteps()
{
    int steps = static_cast<int>(AccumulatedTime / FixedTimeStep);
    return std::min(steps, MaxSubSteps);
}

// 시뮬레이션 시작 전 준비
void UPhysicsSystem::PrepareSimulation()
{
    bIsSimulating = true;

    //비유효 ObejectRef 정리는 안함. Unregister시에 진행(ID를 통한 인터페이스 유지)

    // 작업 큐 차례대로 실행
    ProcessJobQueue();

    //JobQeueu 클리어
    JobQueue.Clear();
    //JobPool 클리어
    JobPool.Reset();
}

// 단일 서브스텝 시뮬레이션
float UPhysicsSystem::SimulateSubstep(const float StepTime)
{
    //가장 적은 시뮬시간
    float MinSimulatedTimeRatio = 1.0f;
    
    // 1. 충돌 
    float CollideTimeRatio = GetCollisionSubsystem()->SimulateCollision(StepTime);
    MinSimulatedTimeRatio = std::min(MinSimulatedTimeRatio, CollideTimeRatio);

    // 시뮬레이션 시간 업데이트
    // Tick 시간 클램핑 ( 로직 처리에 안정성을 주기위한 최소 틱시간 결정)
    float SimualtedTime = std::max(MinSubStepTickTime, StepTime * MinSimulatedTimeRatio);


    //공통 물리 배치 시뮬레이션
    // 중력 적용
    BatchApplyGravity(Gravity, SimualtedTime);

    // 드래그 적용
    BatchApplyDrag(SimualtedTime);

    // 속도 적분 (위치 업데이트)
    BatchIntegrateVelocity(SimualtedTime);

    // 물리 Tick
    BatchPhysicsTick(SimualtedTime);

    return SimualtedTime;
}


// 시뮬레이션 완료 후 상태 적용
void UPhysicsSystem::FinalizeSimulation()
{
    bIsSimulating = false;

    BatchSynchronizeState();
}

#pragma endregion

void UPhysicsSystem::ProcessJobQueue()
{
    // Job Queue를 순차적으로 처리
    while (!JobQueue.Empty())
    {
        auto request = JobQueue.Front();
        JobQueue.Pop();

        if (request.IsValid())
        {
            // Job 실행 - this를 IPhysicsStateInternal*로 전달
            request.Job->Execute(this);
        }
    }
}

// === 내부 유틸리티 구현 ===

SoAIdx UPhysicsSystem::GetIdx(const SoAID targetID) const
{
    return PhysicsStateSoA.GetIndex(targetID);
}

bool UPhysicsSystem::IsValidTargetID(const SoAID targetID) const
{
    return PhysicsStateSoA.IsValidId(targetID);
}

#pragma region IPhysicsInternal
// === 물리 속성 접근자 ===

float UPhysicsSystem::P_GetMass(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetMass: %u", targetID);
        return 1.0f;  // 기본값 반환
    }

    SoAIdx index = GetIdx(targetID);
    float invMass = PhysicsStateSoA.InvMasses[index];

    // InvMass가 0이면 무한 질량 (Static)
    return (invMass > KINDA_SMALL) ? (1.0f / invMass) : FLT_MAX;
}

float UPhysicsSystem::P_GetInvMass(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetInvMass: %u", targetID);
        return 1.0f;
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.InvMasses[index];
}

Vector3 UPhysicsSystem::P_GetRotationalInertia(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetRotationalInertia: %u", targetID);
        return Vector3::One();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR invInertia = PhysicsStateSoA.InvRotationalInertias[index];

    // InvRotationalInertia를 RotationalInertia로 변환
    Vector3 result;
    XMFLOAT3 invInertiaFloat;
    XMStoreFloat3(&invInertiaFloat, invInertia);

    result.x = (abs(invInertiaFloat.x) > KINDA_SMALL) ? (1.0f / invInertiaFloat.x) : FLT_MAX;
    result.y = (abs(invInertiaFloat.y) > KINDA_SMALL) ? (1.0f / invInertiaFloat.y) : FLT_MAX;
    result.z = (abs(invInertiaFloat.z) > KINDA_SMALL) ? (1.0f / invInertiaFloat.z) : FLT_MAX;

    return result;
}

Vector3 UPhysicsSystem::P_GetInvRotationalInertia(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetInvRotationalInertia: %u", targetID);
        return Vector3::One();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR invInertia = PhysicsStateSoA.InvRotationalInertias[index];

    Vector3 result;
    XMFLOAT3 invInertiaFloat;
    XMStoreFloat3(&invInertiaFloat, invInertia);

    result.x = invInertiaFloat.x;
    result.y = invInertiaFloat.y;
    result.z = invInertiaFloat.z;

    return result;
}

float UPhysicsSystem::P_GetRestitution(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetRestitution: %u", targetID);
        return 0.5f;
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.Restitutions[index];
}

float UPhysicsSystem::P_GetFrictionStatic(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetFrictionStatic: %u", targetID);
        return 0.5f;
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.FrictionStatics[index];
}

float UPhysicsSystem::P_GetFrictionKinetic(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetFrictionKinetic: %u", targetID);
        return 0.3f;
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.FrictionKinetics[index];
}

float UPhysicsSystem::P_GetGravityScale(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetGravityScale: %u", targetID);
        return 1.0f;
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.GravityScales[index];
}

float UPhysicsSystem::P_GetMaxSpeed(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetMaxSpeed: %u", targetID);
        return -1.0f;  // 무제한
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.MaxSpeeds[index];
}

float UPhysicsSystem::P_GetMaxAngularSpeed(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetMaxAngularSpeed: %u", targetID);
        return -1.0f;  // 무제한
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.MaxAngularSpeeds[index];
}

// === 운동 상태 접근자 ===

Vector3 UPhysicsSystem::P_GetVelocity(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetVelocity: %u", targetID);
        return Vector3::Zero();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR velocity = PhysicsStateSoA.Velocities[index];

    Vector3 result;
    XMFLOAT3 velocityFloat;
    XMStoreFloat3(&velocityFloat, velocity);

    result.x = velocityFloat.x;
    result.y = velocityFloat.y;
    result.z = velocityFloat.z;

    return result;
}

Vector3 UPhysicsSystem::P_GetAngularVelocity(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetAngularVelocity: %u", targetID);
        return Vector3::Zero();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR angularVelocity = PhysicsStateSoA.AngularVelocities[index];

    Vector3 result;
    XMFLOAT3 angularVelocityFloat;
    XMStoreFloat3(&angularVelocityFloat, angularVelocity);

    result.x = angularVelocityFloat.x;
    result.y = angularVelocityFloat.y;
    result.z = angularVelocityFloat.z;

    return result;
}

Vector3 UPhysicsSystem::P_GetAccumulatedForce(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetAccumulatedForce: %u", targetID);
        return Vector3::Zero();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR force = PhysicsStateSoA.AccumulatedForces[index];

    Vector3 result;
    XMFLOAT3 forceFloat;
    XMStoreFloat3(&forceFloat, force);

    result.x = forceFloat.x;
    result.y = forceFloat.y;
    result.z = forceFloat.z;

    return result;
}

Vector3 UPhysicsSystem::P_GetAccumulatedTorque(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetAccumulatedTorque: %u", targetID);
        return Vector3::Zero();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR torque = PhysicsStateSoA.AccumulatedTorques[index];

    Vector3 result;
    XMFLOAT3 torqueFloat;
    XMStoreFloat3(&torqueFloat, torque);

    result.x = torqueFloat.x;
    result.y = torqueFloat.y;
    result.z = torqueFloat.z;

    return result;
}

// === 트랜스폼 접근자 ===

FTransform UPhysicsSystem::P_GetWorldTransform(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetWorldTransform: %u", targetID);
        return FTransform();
    }

    SoAIdx index = GetIdx(targetID);

    FTransform result;

    // Position
    XMVECTOR position = PhysicsStateSoA.WorldPosition[index];
    XMFLOAT3 positionFloat;
    XMStoreFloat3(&positionFloat, position);
    result.Position = Vector3(positionFloat.x, positionFloat.y, positionFloat.z);

    // Rotation
    XMVECTOR rotation = PhysicsStateSoA.WorldRotationQuat[index];
    XMFLOAT4 rotationFloat;
    XMStoreFloat4(&rotationFloat, rotation);
    result.Rotation = Quaternion(rotationFloat.x, rotationFloat.y, rotationFloat.z, rotationFloat.w);

    // Scale
    XMVECTOR scale = PhysicsStateSoA.WorldScale[index];
    XMFLOAT3 scaleFloat;
    XMStoreFloat3(&scaleFloat, scale);
    result.Scale = Vector3(scaleFloat.x, scaleFloat.y, scaleFloat.z);

    return result;
}

Vector3 UPhysicsSystem::P_GetWorldPosition(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetWorldPosition: %u", targetID);
        return Vector3::Zero();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR position = PhysicsStateSoA.WorldPosition[index];

    Vector3 result;
    XMFLOAT3 positionFloat;
    XMStoreFloat3(&positionFloat, position);

    result.x = positionFloat.x;
    result.y = positionFloat.y;
    result.z = positionFloat.z;

    return result;
}

Quaternion UPhysicsSystem::P_GetWorldRotation(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetWorldRotation: %u", targetID);
        return Quaternion::Identity();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR rotation = PhysicsStateSoA.WorldRotationQuat[index];

    XMFLOAT4 rotationFloat;
    XMStoreFloat4(&rotationFloat, rotation);

    return Quaternion(rotationFloat.x, rotationFloat.y, rotationFloat.z, rotationFloat.w);
}

Vector3 UPhysicsSystem::P_GetWorldScale(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetWorldScale: %u", targetID);
        return Vector3::One();
    }

    SoAIdx index = GetIdx(targetID);
    XMVECTOR scale = PhysicsStateSoA.WorldScale[index];

    Vector3 result;
    XMFLOAT3 scaleFloat;
    XMStoreFloat3(&scaleFloat, scale);

    result.x = scaleFloat.x;
    result.y = scaleFloat.y;
    result.z = scaleFloat.z;

    return result;
}

// === 상태 타입 및 마스크 접근자 ===

EPhysicsType UPhysicsSystem::P_GetPhysicsType(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetPhysicsType: %u", targetID);
        return EPhysicsType::Dynamic;
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.PhysicsTypes[index];
}

FPhysicsMask UPhysicsSystem::P_GetPhysicsMask(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetPhysicsMask: %u", targetID);
        return FPhysicsMask(FPhysicsMask::MASK_NONE);
    }

    SoAIdx index = GetIdx(targetID);
    return PhysicsStateSoA.PhysicsMasks[index];
}

// === 활성화 제어 접근자 ===

bool UPhysicsSystem::P_IsPhysicsActive(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_IsPhysicsActive: %u", targetID);
        return false;
    }

    FPhysicsMask Mask = P_GetPhysicsMask(targetID);
    return Mask.HasFlag(FPhysicsMask::MASK_ACTIVATION);
}

bool UPhysicsSystem::P_IsCollisionActive(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_IsCollisionActive: %u", targetID);
        return false;
    }

    FPhysicsMask Mask = P_GetPhysicsMask(targetID);
    return Mask.HasFlag(FPhysicsMask::MASK_COLLISION_ENABLED);
}
#pragma endregion

///////////////////////////////////////////////////////////

void UPhysicsSystem::PrintDebugInfo()
{
#ifdef _DEBUG
    LOG_NORMAL("Current Active PhysicsObejct : [%03d]", PhysicsStateSoA.GetActiveObjectCount());
#endif
}
