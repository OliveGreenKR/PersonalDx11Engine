#include "PhysicsSystem.h"
#include "CollisionProcessor.h"
#include <algorithm>
#include "Debug.h"
#include "ConfigReadManager.h"

#pragma region Constructors and PhysicsObjects LifeCycle Management
UPhysicsSystem::UPhysicsSystem()
    : PhysicsStateSoA(128)
	, JobPool(1 * 1024 * 1024)  //기본 1MB
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
        JobPool.Initialize(InitialPhysicsJobPoolSizeMB * 1024 * 1024);
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
    UConfigReadManager::Get()->GetValue("InitialPhysicsJobPoolSizeMB", InitialPhysicsJobPoolSizeMB);
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
        float SimulatedTime = SimulateSubstep(TimeStep);
        TimeStep -= SimulatedTime;

        // 시간 전부 사용- 서브스텝 종료
        if (TimeStep < KINDA_SMALL && i > MinSubSteps)
        {
            break;
        }
        AccumulatedTime -= SimulatedTime;
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

    // 1. 게임 → 물리 동기화 
    SyncGameToPhysics();

    // 2. 작업 큐 차례대로 실행
    ProcessJobQueue();

    // 3. 비유효 객체 정리
    PhysicsStateSoA.CleanupExpiredObjectRefs();

    // 4. JobQueue 클리어
    JobQueue.Clear();

    // 5. JobPool 클리어
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

void UPhysicsSystem::FinalizeSimulation()
{
    // 시뮬레이션 플래그 해제
    bIsSimulating = false;

    // 물리 → 게임 동기화 
    SyncPhysicsToGame();
}

#pragma endregion

#pragma region JobSystem
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
#pragma endregion

#pragma region Inner Helper
SoAIdx UPhysicsSystem::GetIdx(const SoAID targetID) const
{
    return PhysicsStateSoA.GetIndex(targetID);
}

bool UPhysicsSystem::IsValidTargetID(const PhysicsID targetID) const
{
    SoAID soaID = static_cast<SoAID>(targetID);
    return PhysicsStateSoA.IsValidSlotID(soaID);
}
#pragma endregion

#pragma region Synchronization System Implementation (SoA Batch Optimized)

void UPhysicsSystem::SyncGameToPhysics()
{
    // 각 더티 플래그별로 개별 순회
    BatchSyncHighFrequencyData();
    BatchSyncMidFrequencyData();
    BatchSyncLowFrequencyData();

    // 모든 더티 플래그 일괄 정리
    BatchClearAllDirtyFlags();
}

void UPhysicsSystem::SyncPhysicsToGame()
{
    BatchSyncPhysicsResults();
}

void UPhysicsSystem::BatchSyncHighFrequencyData()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA.ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // High Frequency 더티 플래그 확인
            FPhysicsDataDirtyFlags dirtyFlags = physicsObject->GetDirtyFlags();
            if (!dirtyFlags.HasHighFreq())
                continue;

            // Transform 데이터 동기화
            FHighFrequencyData data = physicsObject->GetHighFrequencyData();

            PhysicsStateSoA.WorldPosition[i] = XMVectorSet(
                data.Position.x, data.Position.y, data.Position.z, 1.0f);
            PhysicsStateSoA.WorldRotationQuat[i] = XMVectorSet(
                data.Rotation.x, data.Rotation.y, data.Rotation.z, data.Rotation.w);
            PhysicsStateSoA.WorldScale[i] = XMVectorSet(
                data.Scale.x, data.Scale.y, data.Scale.z, 1.0f);
        }
    }
}

void UPhysicsSystem::BatchSyncMidFrequencyData()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA.ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // Mid Frequency 더티 플래그 확인
            FPhysicsDataDirtyFlags dirtyFlags = physicsObject->GetDirtyFlags();
            if (!dirtyFlags.HasMidFreq())
                continue;

            // Type, Mask 데이터 동기화
            FMidFrequencyData data = physicsObject->GetMidFrequencyData();

            PhysicsStateSoA.PhysicsTypes[i] = data.PhysicsType;
            PhysicsStateSoA.PhysicsMasks[i] = data.PhysicsMask;
        }
    }
}

void UPhysicsSystem::BatchSyncLowFrequencyData()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA.ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // Low Frequency 더티 플래그 확인
            FPhysicsDataDirtyFlags dirtyFlags = physicsObject->GetDirtyFlags();
            if (!dirtyFlags.HasLowFreq())
                continue;

            // Properties 데이터 동기화
            FLowFrequencyData data = physicsObject->GetLowFrequencyData();

            PhysicsStateSoA.InvMasses[i] = data.InvMass;
            PhysicsStateSoA.FrictionKinetics[i] = data.FrictionKinetic;
            PhysicsStateSoA.FrictionStatics[i] = data.FrictionStatic;
            PhysicsStateSoA.Restitutions[i] = data.Restitution;
            PhysicsStateSoA.InvRotationalInertias[i] = XMVectorSet(
                data.InvRotationalInertia.x,
                data.InvRotationalInertia.y,
                data.InvRotationalInertia.z,
                0.0f);
            PhysicsStateSoA.MaxSpeeds[i] = data.MaxSpeed;
            PhysicsStateSoA.MaxAngularSpeeds[i] = data.MaxAngularSpeed;
            PhysicsStateSoA.GravityScales[i] = data.GravityScale;
        }
    }
}

void UPhysicsSystem::BatchSyncPhysicsResults()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA.ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // 물리 시뮬레이션 결과 데이터 구성
            FPhysicsToGameData physicsResults;

            // SIMD 최적화된 데이터 읽기
            XMVECTOR velocity = PhysicsStateSoA.Velocities[i];
            XMVECTOR angularVelocity = PhysicsStateSoA.AngularVelocities[i];
            XMVECTOR position = PhysicsStateSoA.WorldPosition[i];
            XMVECTOR rotation = PhysicsStateSoA.WorldRotationQuat[i];
            XMVECTOR scale = PhysicsStateSoA.WorldScale[i];

            XMFLOAT3 velocityFloat, angularVelocityFloat, positionFloat, scaleFloat;
            XMFLOAT4 rotationFloat;

            XMStoreFloat3(&velocityFloat, velocity);
            XMStoreFloat3(&angularVelocityFloat, angularVelocity);
            XMStoreFloat3(&positionFloat, position);
            XMStoreFloat4(&rotationFloat, rotation);
            XMStoreFloat3(&scaleFloat, scale);

            physicsResults.Velocity = Vector3(velocityFloat.x, velocityFloat.y, velocityFloat.z);
            physicsResults.AngularVelocity = Vector3(angularVelocityFloat.x, angularVelocityFloat.y, angularVelocityFloat.z);
            physicsResults.ResultPosition = Vector3(positionFloat.x, positionFloat.y, positionFloat.z);
            physicsResults.ResultRotation = Quaternion(rotationFloat.x, rotationFloat.y, rotationFloat.z, rotationFloat.w);
            physicsResults.ResultScale = Vector3(scaleFloat.x, scaleFloat.y, scaleFloat.z);

            // 게임 객체로 결과 전송
            physicsObject->ReceivePhysicsResults(physicsResults);
        }
    }
}

void UPhysicsSystem::BatchClearAllDirtyFlags()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당된 슬롯만 처리 (활성화 여부 무관하게 더티 플래그 정리)
            if (!PhysicsStateSoA.IsValidSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA.ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // 모든 더티 플래그 일괄 정리
            FPhysicsDataDirtyFlags allFlags(FPhysicsDataDirtyFlags::FLAG_ALL);
            physicsObject->MarkDataClean(allFlags);
        }
    }
}

#pragma endregion

#pragma region IPhysicsInternal

#pragma region Getter
// === 물리 속성 접근자 ===

float UPhysicsSystem::P_GetMass(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetMass: %u", targetID);
        return KINDA_LARGE;  // 기본값 반환
    }

    SoAIdx index = GetIdx(targetID);
    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        return KINDA_LARGE;
    }
    float invMass = PhysicsStateSoA.InvMasses[index];

    // InvMass가 0이면 무한 질량 (Static)
    return (invMass > KINDA_SMALL) ? (1.0f / invMass) : KINDA_LARGE;
}

float UPhysicsSystem::P_GetInvMass(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetInvMass: %u", targetID);
        return KINDA_SMALL;
    }

    SoAIdx index = GetIdx(targetID);
    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        return KINDA_SMALL;
    }

    return PhysicsStateSoA.InvMasses[index];
}

Vector3 UPhysicsSystem::P_GetRotationalInertia(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetRotationalInertia: %u", targetID);
        return KINDA_LARGE * Vector3::One();
    }

    SoAIdx index = GetIdx(targetID);
    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        return KINDA_LARGE * Vector3::One();
    }
    XMVECTOR invInertia = PhysicsStateSoA.InvRotationalInertias[index];

    // InvRotationalInertia를 RotationalInertia로 변환
    Vector3 result;
    XMFLOAT3 invInertiaFloat;
    XMStoreFloat3(&invInertiaFloat, invInertia);

    result.x = (abs(invInertiaFloat.x) > KINDA_SMALL) ? (1.0f / invInertiaFloat.x) : KINDA_LARGE;
    result.y = (abs(invInertiaFloat.y) > KINDA_SMALL) ? (1.0f / invInertiaFloat.y) : KINDA_LARGE;
    result.z = (abs(invInertiaFloat.z) > KINDA_SMALL) ? (1.0f / invInertiaFloat.z) : KINDA_LARGE;

    return result;
}

Vector3 UPhysicsSystem::P_GetInvRotationalInertia(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_GetInvRotationalInertia: %u", targetID);
        return KINDA_SMALL * Vector3::One();
    }

    SoAIdx index = GetIdx(targetID);
    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        return KINDA_SMALL * Vector3::One();
    }
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
#pragma endregion
#pragma region Setter
// === 운동 상태 설정자 (Static 타입 보호) ===

void UPhysicsSystem::P_SetVelocity(PhysicsID targetID, const Vector3& velocity)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetVelocity: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_SetVelocity blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    // 유효성 검사
    XMVECTOR velocityVec = XMVectorSet(velocity.x, velocity.y, velocity.z, 0.0f);
    if (!IsValidLinearVelocity(velocityVec))
    {
        LOG_WARNING("Invalid velocity value for PhysicsID %u: (%.3f, %.3f, %.3f)",
                    targetID, velocity.x, velocity.y, velocity.z);
        return;
    }

    PhysicsStateSoA.Velocities[index] = velocityVec;
}

void UPhysicsSystem::P_AddVelocity(PhysicsID targetID, const Vector3& deltaVelocity)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_AddVelocity: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_AddVelocity blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR deltaVec = XMVectorSet(deltaVelocity.x, deltaVelocity.y, deltaVelocity.z, 0.0f);
    XMVECTOR currentVelocity = PhysicsStateSoA.Velocities[index];
    XMVECTOR newVelocity = XMVectorAdd(currentVelocity, deltaVec);

    // 유효성 검사
    if (!IsValidLinearVelocity(newVelocity))
    {
        LOG_WARNING("Invalid result velocity for PhysicsID %u after adding delta", targetID);
        return;
    }

    PhysicsStateSoA.Velocities[index] = newVelocity;
}

void UPhysicsSystem::P_SetAngularVelocity(PhysicsID targetID, const Vector3& angularVelocity)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetAngularVelocity: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_SetAngularVelocity blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR angularVelVec = XMVectorSet(angularVelocity.x, angularVelocity.y, angularVelocity.z, 0.0f);
    if (!IsValidAngularVelocity(angularVelVec))
    {
        LOG_WARNING("Invalid angular velocity value for PhysicsID %u: (%.3f, %.3f, %.3f)",
                    targetID, angularVelocity.x, angularVelocity.y, angularVelocity.z);
        return;
    }

    PhysicsStateSoA.AngularVelocities[index] = angularVelVec;
}

void UPhysicsSystem::P_AddAngularVelocity(PhysicsID targetID, const Vector3& deltaAngularVelocity)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_AddAngularVelocity: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_AddAngularVelocity blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR deltaVec = XMVectorSet(deltaAngularVelocity.x, deltaAngularVelocity.y, deltaAngularVelocity.z, 0.0f);
    XMVECTOR currentAngularVel = PhysicsStateSoA.AngularVelocities[index];
    XMVECTOR newAngularVel = XMVectorAdd(currentAngularVel, deltaVec);

    if (!IsValidAngularVelocity(newAngularVel))
    {
        LOG_WARNING("Invalid result angular velocity for PhysicsID %u after adding delta", targetID);
        return;
    }

    PhysicsStateSoA.AngularVelocities[index] = newAngularVel;
}

// === 트랜스폼 설정자 (Static 타입 보호) ===

void UPhysicsSystem::P_SetWorldPosition(PhysicsID targetID, const Vector3& position)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetWorldPosition: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_SetWorldPosition blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR positionVec = XMVectorSet(position.x, position.y, position.z, 1.0f);
    PhysicsStateSoA.WorldPosition[index] = positionVec;
}

void UPhysicsSystem::P_SetWorldRotation(PhysicsID targetID, const Quaternion& rotation)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetWorldRotation: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_SetWorldRotation blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR rotationVec = XMVectorSet(rotation.x, rotation.y, rotation.z, rotation.w);
    // 쿼터니언 정규화
    rotationVec = XMQuaternionNormalize(rotationVec);

    PhysicsStateSoA.WorldRotationQuat[index] = rotationVec;
}

void UPhysicsSystem::P_SetWorldScale(PhysicsID targetID, const Vector3& scale)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetWorldScale: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_SetWorldScale blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    // 스케일 유효성 검사 (0 또는 음수 방지)
    Vector3 validScale = scale;
    if (validScale.x <= KINDA_SMALL) validScale.x = KINDA_SMALL;
    if (validScale.y <= KINDA_SMALL) validScale.y = KINDA_SMALL;
    if (validScale.z <= KINDA_SMALL) validScale.z = KINDA_SMALL;

    XMVECTOR scaleVec = XMVectorSet(validScale.x, validScale.y, validScale.z, 1.0f);
    PhysicsStateSoA.WorldScale[index] = scaleVec;
}

// === 힘/충격 적용 (Static 타입 보호) ===

void UPhysicsSystem::P_ApplyForce(PhysicsID targetID, const Vector3& force, const Vector3& location)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_ApplyForce: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_ApplyForce blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR forceVec = XMVectorSet(force.x, force.y, force.z, 0.0f);
    if (!IsValidForce(forceVec))
    {
        LOG_WARNING("Invalid force value for PhysicsID %u at location", targetID);
        return;
    }

    // 중심에서 적용점까지의 벡터
    Vector3 centerOfMass = P_GetWorldPosition(targetID);
    Vector3 radius = location - centerOfMass;

    // 힘을 누적 힘에 추가
    XMVECTOR currentForce = PhysicsStateSoA.AccumulatedForces[index];
    PhysicsStateSoA.AccumulatedForces[index] = XMVectorAdd(currentForce, forceVec);

    // 토크 계산 및 추가 (radius × force)
    XMVECTOR radiusVec = XMVectorSet(radius.x, radius.y, radius.z, 0.0f);
    XMVECTOR torqueVec = XMVector3Cross(radiusVec, forceVec);

    if (IsValidTorque(torqueVec))
    {
        XMVECTOR currentTorque = PhysicsStateSoA.AccumulatedTorques[index];
        PhysicsStateSoA.AccumulatedTorques[index] = XMVectorAdd(currentTorque, torqueVec);
    }
}

void UPhysicsSystem::P_ApplyImpulse(PhysicsID targetID, const Vector3& impulse, const Vector3& location)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_ApplyImpulse: %u", targetID);
        return;
    }

    // Static 타입 체크
    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    if (PhysicsStateSoA.PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_ApplyImpulse blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    XMVECTOR impulseVec = XMVectorSet(impulse.x, impulse.y, impulse.z, 0.0f);
    if (!IsValidForce(impulseVec))
    {
        LOG_WARNING("Invalid impulse value for PhysicsID %u at location", targetID);
        return;
    }

    // 선형 충격 적용
    float invMass = PhysicsStateSoA.InvMasses[index];
    XMVECTOR deltaVelocity = XMVectorScale(impulseVec, invMass);

    XMVECTOR currentVelocity = PhysicsStateSoA.Velocities[index];
    XMVECTOR newVelocity = XMVectorAdd(currentVelocity, deltaVelocity);

    if (IsValidLinearVelocity(newVelocity))
    {
        PhysicsStateSoA.Velocities[index] = newVelocity;
    }

    // 각속도 충격 적용
    Vector3 centerOfMass = P_GetWorldPosition(targetID);
    Vector3 radius = location - centerOfMass;

    XMVECTOR radiusVec = XMVectorSet(radius.x, radius.y, radius.z, 0.0f);
    XMVECTOR angularImpulse = XMVector3Cross(radiusVec, impulseVec);

    if (IsValidTorque(angularImpulse))
    {
        XMVECTOR invInertia = PhysicsStateSoA.InvRotationalInertias[index];
        XMVECTOR deltaAngularVel = XMVectorMultiply(angularImpulse, invInertia);

        XMVECTOR currentAngularVel = PhysicsStateSoA.AngularVelocities[index];
        XMVECTOR newAngularVel = XMVectorAdd(currentAngularVel, deltaAngularVel);

        if (IsValidAngularVelocity(newAngularVel))
        {
            PhysicsStateSoA.AngularVelocities[index] = newAngularVel;
        }
    }
}


// === 물리 속성 설정자 ===

void UPhysicsSystem::P_SetMass(PhysicsID targetID, float mass)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetMass: %u", targetID);
        return;
    }

    if (mass <= KINDA_SMALL)
    {
        LOG_WARNING("Invalid mass value for PhysicsID %u: %f", targetID, mass);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.InvMasses[index] = 1.0f / mass;
}

void UPhysicsSystem::P_SetInvMass(PhysicsID targetID, float invMass)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetInvMass: %u", targetID);
        return;
    }

    if (invMass < 0.0f)
    {
        LOG_WARNING("Invalid inverse mass value for PhysicsID %u: %f", targetID, invMass);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.InvMasses[index] = invMass;
}

void UPhysicsSystem::P_SetRotationalInertia(PhysicsID targetID, const Vector3& rotationalInertia)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetRotationalInertia: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    // 회전 관성을 역수로 변환하여 저장
    Vector3 invInertia;
    invInertia.x = (rotationalInertia.x > KINDA_SMALL) ? (1.0f / rotationalInertia.x) : 0.0f;
    invInertia.y = (rotationalInertia.y > KINDA_SMALL) ? (1.0f / rotationalInertia.y) : 0.0f;
    invInertia.z = (rotationalInertia.z > KINDA_SMALL) ? (1.0f / rotationalInertia.z) : 0.0f;

    XMVECTOR invInertiaVec = XMVectorSet(invInertia.x, invInertia.y, invInertia.z, 0.0f);
    PhysicsStateSoA.InvRotationalInertias[index] = invInertiaVec;
}

void UPhysicsSystem::P_SetInvRotationalInertia(PhysicsID targetID, const Vector3& invRotationalInertia)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetInvRotationalInertia: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    XMVECTOR invInertiaVec = XMVectorSet(invRotationalInertia.x, invRotationalInertia.y, invRotationalInertia.z, 0.0f);
    PhysicsStateSoA.InvRotationalInertias[index] = invInertiaVec;
}

void UPhysicsSystem::P_SetRestitution(PhysicsID targetID, float restitution)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetRestitution: %u", targetID);
        return;
    }

    // 반발계수는 0.0 ~ 1.0 범위로 클램핑
    float clampedRestitution = Math::Clamp(restitution, 0.0f, 1.0f);

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.Restitutions[index] = clampedRestitution;
}

void UPhysicsSystem::P_SetFrictionStatic(PhysicsID targetID, float frictionStatic)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetFrictionStatic: %u", targetID);
        return;
    }

    float clampedFriction = Math::Max(frictionStatic, 0.0f);

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.FrictionStatics[index] = clampedFriction;
}

void UPhysicsSystem::P_SetFrictionKinetic(PhysicsID targetID, float frictionKinetic)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetFrictionKinetic: %u", targetID);
        return;
    }

    float clampedFriction = Math::Max(frictionKinetic, 0.0f);

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.FrictionKinetics[index] = clampedFriction;
}

void UPhysicsSystem::P_SetGravityScale(PhysicsID targetID, float gravityScale)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetGravityScale: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.GravityScales[index] = gravityScale;
}

void UPhysicsSystem::P_SetMaxSpeed(PhysicsID targetID, float maxSpeed)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetMaxSpeed: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.MaxSpeeds[index] = maxSpeed;
}

void UPhysicsSystem::P_SetMaxAngularSpeed(PhysicsID targetID, float maxAngularSpeed)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetMaxAngularSpeed: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.MaxAngularSpeeds[index] = maxAngularSpeed;
}

// === 상태 타입 및 마스크 설정자 ===

void UPhysicsSystem::P_SetPhysicsType(PhysicsID targetID, EPhysicsType physicsType)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetPhysicsType: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.PhysicsTypes[index] = physicsType;
}

void UPhysicsSystem::P_SetPhysicsMask(PhysicsID targetID, const FPhysicsMask& physicsMask)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetPhysicsMask: %u", targetID);
        return;
    }

    SoAID soaID = static_cast<SoAID>(targetID);
    SoAIdx index = GetIdx(soaID);

    PhysicsStateSoA.PhysicsMasks[index] = physicsMask;
}

// === 활성화 제어 설정자 ===

void UPhysicsSystem::P_SetPhysicsActive(PhysicsID targetID, bool bActive)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_SetPhysicsActive: %u", targetID);
        return;
    }

    FPhysicsMask mask = P_GetPhysicsMask(targetID);

    if (bActive)
    {
        mask.SetFlag(FPhysicsMask::MASK_ACTIVATION);
    }
    else
    {
        mask.ClearFlag(FPhysicsMask::MASK_ACTIVATION);
    }

    P_SetPhysicsMask(targetID, mask);
}
#pragma endregion

#pragma region Batching Physis Simulation

// === 배치 연산 구현 ===

void UPhysicsSystem::BatchApplyGravity(const Vector3& gravity, float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    // 중력은 이미 가속도(m/s²)이므로 deltaTime을 곱해서 속도 변화량으로 변환
    XMVECTOR gravityVec = XMVectorSet(gravity.x, gravity.y, gravity.z, 0.0f);
    XMVECTOR deltaTimeVec = XMVectorReplicate(deltaTime);

    // 올바른 계산: gravity(가속도) * deltaTime = 속도 변화량
    XMVECTOR gravityVelocityDelta = XMVectorMultiply(gravityVec, deltaTimeVec);

    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i) || PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // 중력 적용 조건 확인
            if (PhysicsStateSoA.PhysicsMasks[i].HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED) &&
                PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Dynamic)
            {
                float invMass = PhysicsStateSoA.InvMasses[i];
                if (invMass <= KINDA_SMALL)  // Static 객체는 무한 질량
                    continue;

                // 중력 스케일 적용
                float gravityScale = PhysicsStateSoA.GravityScales[i];
                XMVECTOR scaledGravityDelta = XMVectorScale(gravityVelocityDelta, gravityScale);

                // 직접 속도에 변화량 적용 (질량은 이미 중력에 반영되어 있음)
                // 실제 물리에서는 모든 객체가 같은 중력 가속도를 받음
                XMVECTOR currentVelocity = PhysicsStateSoA.Velocities[i];
                XMVECTOR newVelocity = XMVectorAdd(currentVelocity, scaledGravityDelta);

                if (IsValidLinearVelocity(newVelocity))
                {
                    PhysicsStateSoA.Velocities[i] = newVelocity;
                }
            }
        }
    }
}

void UPhysicsSystem::BatchIntegrateVelocity(float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    XMVECTOR deltaTimeVec = XMVectorReplicate(deltaTime);

    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i) || PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // 선형 속도 적분 + 속도 제한 (Position += Velocity * deltaTime)
            XMVECTOR velocity = PhysicsStateSoA.Velocities[i];
            if (IsValidLinearVelocity(velocity))
            {
                // 속도 제한 적용 (적분 전에)
                float maxSpeed = PhysicsStateSoA.MaxSpeeds[i];
                if (maxSpeed > 0.0f)  // 음수는 무제한
                {
                    ClampLinearVelocity(maxSpeed, velocity);
                    PhysicsStateSoA.Velocities[i] = velocity;  // 제한된 속도 저장
                }

                XMVECTOR currentPosition = PhysicsStateSoA.WorldPosition[i];
                XMVECTOR deltaPosition = XMVectorMultiply(velocity, deltaTimeVec);
                XMVECTOR newPosition = XMVectorAdd(currentPosition, deltaPosition);

                PhysicsStateSoA.WorldPosition[i] = newPosition;
            }

            // 각속도 적분 + 각속도 제한 (Rotation += AngularVelocity * deltaTime)
            XMVECTOR angularVelocity = PhysicsStateSoA.AngularVelocities[i];
            if (IsValidAngularVelocity(angularVelocity))
            {
                // 각속도 제한 적용 (적분 전에)
                float maxAngularSpeed = PhysicsStateSoA.MaxAngularSpeeds[i];
                if (maxAngularSpeed > 0.0f)  // 음수는 무제한
                {
                    ClampAngularVelocity(maxAngularSpeed, angularVelocity);
                    PhysicsStateSoA.AngularVelocities[i] = angularVelocity;  // 제한된 각속도 저장
                }

                XMVECTOR currentRotation = PhysicsStateSoA.WorldRotationQuat[i];

                // 각속도를 쿼터니언 회전으로 변환
                XMVECTOR angularDisplacement = XMVectorMultiply(angularVelocity, deltaTimeVec);

                // 각변위의 크기 계산
                XMVECTOR angularMagnitude = XMVector3Length(angularDisplacement);
                float angle;
                XMStoreFloat(&angle, angularMagnitude);

                if (angle > KINDA_SMALL)
                {
                    // 회전축 정규화
                    XMVECTOR axis = XMVectorDivide(angularDisplacement, angularMagnitude);

                    // 각변위를 쿼터니언으로 변환
                    XMVECTOR deltaRotation = XMQuaternionRotationAxis(axis, angle);

                    // 현재 회전에 적용
                    XMVECTOR newRotation = XMQuaternionMultiply(currentRotation, deltaRotation);
                    newRotation = XMQuaternionNormalize(newRotation);

                    PhysicsStateSoA.WorldRotationQuat[i] = newRotation;
                }
            }
        }
    }
}

void UPhysicsSystem::BatchResetForces()
{
    XMVECTOR zeroVector = XMVectorZero();

    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당된 슬롯만 처리 (활성화 여부 무관하게 힘 초기화)
            if (!PhysicsStateSoA.IsValidSlotIndex(i) || PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            PhysicsStateSoA.AccumulatedForces[i] = zeroVector;
            PhysicsStateSoA.AccumulatedTorques[i] = zeroVector;
        }
    }
}

void UPhysicsSystem::BatchApplyDrag(float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    // 개선된 드래그 모델 - 더 명확한 효과를 위한 계수 조정
    const float linearDragCoefficient = 0.85f;    
    const float angularDragCoefficient = 0.80f;   // 각속도는 더 강한 드래그

    // 지수적 감쇠: v_new = v_old * (coefficient ^ deltaTime)
    // deltaTime이 작을 때도 효과가 보이도록 계수를 낮춤
    float linearDragFactor = powf(linearDragCoefficient, deltaTime);
    float angularDragFactor = powf(angularDragCoefficient, deltaTime);

    XMVECTOR linearDragVec = XMVectorReplicate(linearDragFactor);
    XMVECTOR angularDragVec = XMVectorReplicate(angularDragFactor);

    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i) || PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // Dynamic 타입만 드래그 적용
            if (PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Dynamic)
            {
                // 선형 속도 드래그 적용
                XMVECTOR currentVelocity = PhysicsStateSoA.Velocities[i];
                if (IsValidLinearVelocity(currentVelocity))
                {
                    // 속도가 매우 작으면 완전히 정지시켜 진동 방지
                    float velocityMagnitude = XMVector3Length(currentVelocity).m128_f32[0];
                    if (velocityMagnitude < 0.01f * ONE_METER)  
                    {
                        PhysicsStateSoA.Velocities[i] = XMVectorZero();
                    }
                    else
                    {
                        XMVECTOR newVelocity = XMVectorMultiply(currentVelocity, linearDragVec);
                        PhysicsStateSoA.Velocities[i] = newVelocity;
                    }
                }

                // 각속도 드래그 적용
                XMVECTOR currentAngularVel = PhysicsStateSoA.AngularVelocities[i];
                if (IsValidAngularVelocity(currentAngularVel))
                {
                    // 각속도가 매우 작으면 완전히 정지시켜 진동 방지
                    float angularMagnitude = XMVector3Length(currentAngularVel).m128_f32[0];
                    if (angularMagnitude < 0.1f)  // 약 5.7도/초 이하면 정지
                    {
                        PhysicsStateSoA.AngularVelocities[i] = XMVectorZero();
                    }
                    else
                    {
                        XMVECTOR newAngularVel = XMVectorMultiply(currentAngularVel, angularDragVec);
                        PhysicsStateSoA.AngularVelocities[i] = newAngularVel;
                    }
                }
            }
        }
    }
}

void UPhysicsSystem::BatchPhysicsTick(float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA.GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA.GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA.IsValidActiveSlotIndex(i) || PhysicsStateSoA.PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // ObjectReferences를 통한 IPhysicsObject::TickPhysics 호출
            if (i < PhysicsStateSoA.ObjectReferences.size())
            {
                if (auto physicsObject = PhysicsStateSoA.ObjectReferences[i].lock())
                {
                    physicsObject->TickPhysics(deltaTime);
                }
            }
        }
    }
}
#pragma endregion

#pragma region Numeric Stability , Clamp States Helper
// === 수치 안정성 헬퍼 메서드 구현 ===

bool UPhysicsSystem::IsValidLinearVelocity(const XMVECTOR& InVelocity)
{
    // 속도 크기 계산
    float magnitude = XMVector3Length(InVelocity).m128_f32[0];
    // 최대 허용 속도 검사 (물리적으로 합리적인 범위)
    const float MAX_REASONABLE_VELOCITY = 1000.0f * ONE_METER;  // 1000 m/s (음속의 약 3배)
    return magnitude > KINDA_SMALL && magnitude < MAX_REASONABLE_VELOCITY;
}

bool UPhysicsSystem::IsValidAngularVelocity(const XMVECTOR& InAngularVelocity)
{
    // 각속도 크기 계산
    float magnitude = XMVector3Length(InAngularVelocity).m128_f32[0];
    // 최대 허용 각속도 검사 (라디안/초)
    const float MAX_REASONABLE_ANGULAR_VELOCITY = 100.0f;  // 약 573도/초
    return magnitude > KINDA_SMALL && magnitude < MAX_REASONABLE_ANGULAR_VELOCITY;
}

bool UPhysicsSystem::IsValidForce(const XMVECTOR& InForce)
{
    // 힘의 크기 계산
    float magnitude = XMVector3Length(InForce).m128_f32[0];
    // 최대 허용 힘 검사 (뉴턴)
    const float MAX_REASONABLE_FORCE = 1000000.0f * ONE_METER;  // 1MN (메가뉴턴)
    return magnitude > KINDA_SMALL && magnitude < MAX_REASONABLE_FORCE;
}

bool UPhysicsSystem::IsValidTorque(const XMVECTOR& InTorque)
{
    // 토크의 크기 계산
    float magnitude = XMVector3Length(InTorque).m128_f32[0];
    // 최대 허용 토크 검사 (뉴턴·미터)
    const float MAX_REASONABLE_TORQUE = 100000.0f * ONE_METER;  // 100kN·m
    return magnitude > KINDA_SMALL && magnitude < MAX_REASONABLE_TORQUE;
}

bool UPhysicsSystem::IsValidLinearAcceleration(const XMVECTOR& InAccel)
{
    // 가속도 크기 계산
    float magnitude = XMVector3Length(InAccel).m128_f32[0];
    // 최대 허용 가속도 검사 (m/s²)
    const float MAX_REASONABLE_ACCELERATION = 10000.0f * ONE_METER;  // 약 1000G
    return magnitude > KINDA_SMALL && magnitude < MAX_REASONABLE_ACCELERATION;
}

bool UPhysicsSystem::IsValidAngularAcceleration(const XMVECTOR& InAngularAccel)
{
    // 각가속도 크기 계산
    float magnitude = XMVector3Length(InAngularAccel).m128_f32[0];
    // 최대 허용 각가속도 검사 (라디안/초²)
    const float MAX_REASONABLE_ANGULAR_ACCELERATION = 1000.0f;  // 약 57,000도/초²
    return magnitude > KINDA_SMALL && magnitude < MAX_REASONABLE_ANGULAR_ACCELERATION;
}

void UPhysicsSystem::ClampLinearVelocity(float InMaxSpeed, XMVECTOR& InOutVelocity)
{
    if (InMaxSpeed < 0.0f)
        return;  // 제한 없음

    // 속도 크기 계산
    float magnitude = XMVector3Length(InOutVelocity).m128_f32[0];

    // 극소값 처리
    if (magnitude < KINDA_SMALL)
    {
        InOutVelocity = XMVectorZero();
        return;
    }

    // 최대 속도 초과 시 클램핑
    if (magnitude > InMaxSpeed)
    {
        float scale = InMaxSpeed / magnitude;
        InOutVelocity = XMVectorScale(InOutVelocity, scale);
    }
}

void UPhysicsSystem::ClampAngularVelocity(float InAngularMaxSpeed, XMVECTOR& InOutAngularVelocity)
{
    if (InAngularMaxSpeed <= 0.0f)
        return;  // 제한 없음

    // 각속도 크기 계산
    float magnitude = XMVector3Length(InOutAngularVelocity).m128_f32[0];

    // 극소값 처리
    if (magnitude < KINDA_SMALL)
    {
        InOutAngularVelocity = XMVectorZero();
        return;
    }

    // 최대 각속도 초과 시 클램핑
    if (magnitude > InAngularMaxSpeed)
    {
        float scale = InAngularMaxSpeed / magnitude;
        InOutAngularVelocity = XMVectorScale(InOutAngularVelocity, scale);
    }
}
#pragma endregion

#pragma region Debug
void UPhysicsSystem::PrintDebugInfo()
{
#ifdef _DEBUG
    LOG_NORMAL("Current Active PhysicsObejct : [%03d]", PhysicsStateSoA.GetActiveObjectCount());
#endif
}
#pragma endregion