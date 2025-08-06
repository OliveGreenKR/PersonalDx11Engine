#include "PhysicsSystem.h"
#include "CollisionProcessor.h"
#include <algorithm>
#include "Debug.h"
#include "ConfigReadManager.h"

#pragma region Constructor and Initialization

UPhysicsSystem::~UPhysicsSystem()
{
    Release();
}

void UPhysicsSystem::Initialize()
{
    try
    {
        LoadConfigFromIni();
        CollisionProcessor = std::make_unique<FCollisionProcessor>();
        CollisionProcessor->Initialize(this, this, this);
        PhysicsStateSoA = std::make_unique<FPhysicsStateArrays>(InitialPhysicsObjectCapacity);
        JobPool = std::make_unique<FArenaMemoryPool>(InitialPhysicsJobPoolSizeMB * 1024 * 1024);
		JobQueue = std::make_unique<TCircularQueue<FPhysicsJobRequest>>(InitialPhysicsObjectCapacity);
        CollisionEventQueue = std::make_unique<TCircularQueue<FPhysicsCollisionEvent>>(InitialCollisionEventQueueSize);
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
    JobQueue->Clear();
    //풀 정리
    JobPool->Reset();

    //PhysicsStateSoA는 자동정리
    //충돌 프로세서 정리
    CollisionProcessor->Release();
}

void UPhysicsSystem::LoadConfigFromIni()
{
    UConfigReadManager::Get()->GetValue("InitialCollisionEventQueueSize", InitialCollisionEventQueueSize);
    UConfigReadManager::Get()->GetValue("InitialPhysicsObjectCapacity", InitialPhysicsObjectCapacity);
    UConfigReadManager::Get()->GetValue("InitialPhysicsJobPoolSizeMB", InitialPhysicsJobPoolSizeMB);
    UConfigReadManager::Get()->GetValue("FixedTimeStep", FixedTimeStep);
    UConfigReadManager::Get()->GetValue("MinSubStepTickTime", MinSubStepTickTime);
    UConfigReadManager::Get()->GetValue("MaxSubSteps", MaxSubSteps);
    UConfigReadManager::Get()->GetValue("MaxPhysicsVelocity", MaxPhysicsVelocity);
    UConfigReadManager::Get()->GetValue("MaxPhysicsAngularVelocity", MaxPhysicsAngularVelocity);
    UConfigReadManager::Get()->GetValue("MaxPhysicsForce", MaxPhysicsForce);
    UConfigReadManager::Get()->GetValue("MaxPhysicsTorque", MaxPhysicsTorque);
    UConfigReadManager::Get()->GetValue("MaxPhysicsAcceleration", MaxPhysicsAcceleration);
    UConfigReadManager::Get()->GetValue("MaxPhysicsAngularAcceleration", MaxPhysicsAngularAcceleration);
}

#pragma endregion

#pragma region Public Interface

SoAID UPhysicsSystem::RegisterPhysicsObject(std::shared_ptr<IPhysicsObject>& Object)
{
    if (!Object)
    {
        LOG_WARNING("Invalid PhysicsObject try to Register to PhysiscSystem.");
        return FPhysicsStateArrays::INVALID_ID;
    }

    std::weak_ptr<IPhysicsObject> weakRef = Object;
    SoAID newID = PhysicsStateSoA->AllocateSlot(weakRef);

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
        PhysicsStateSoA->DeallocateSlot(id);
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
        if (TimeStep < KINDA_SMALL)
        {
            break;
        }
        AccumulatedTime -= SimulatedTime;
    }

    // 시뮬레이션 결과 적용
    FinalizeSimulation();
}

void UPhysicsSystem::PrintDebugInfo()
{
#ifdef _DEBUG
    LOG_NORMAL("Current Active PhysicsObejct : [%03d]", PhysicsStateSoA->GetActiveObjectCount());
#endif
}

#pragma endregion

#pragma region Data Access Layer

SoAIdx UPhysicsSystem::GetIdx(const SoAID targetID) const
{
    return PhysicsStateSoA->GetIndex(targetID);
}

bool UPhysicsSystem::IsValidTargetID(const PhysicsID targetID) const
{
    SoAID soaID = static_cast<SoAID>(targetID);
    return PhysicsStateSoA->IsValidSlotID(soaID);
}

std::vector<PhysicsID> UPhysicsSystem::GetActivePhysicsIDs() const
{
    //TODO: 좀더 효율적인 방식 없나
    std::vector<PhysicsID> ActiveIDs;

    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx i = startIdx; i < endIdx; ++i)
    {
        if (PhysicsStateSoA->IsValidActiveSlotIndex(i))
        {
            PhysicsID id = PhysicsStateSoA->GetID(i);
            ActiveIDs.push_back(id);
        }
    }

    return ActiveIDs;
}

#pragma endregion

#pragma region IPhysicsEventDispatcher Implementation

void UPhysicsSystem::AddCollisionEvent(const FPhysicsCollisionEvent& Event)
{
    if (!CollisionEventQueue)
    {
        LOG_ERROR("CollisionEventQueue is not initialized");
        return;
    }

    CollisionEventQueue->Push(Event);
}

void UPhysicsSystem::SyncPhysicsEvents()
{
    if (!CollisionEventQueue || CollisionEventQueue->Empty())
    {
        return;
    }

    // 1. PhysicsID별 원본 물리 이벤트 그룹화
    std::unordered_map<PhysicsID, std::vector<FPhysicsCollisionEvent>> EventGroups;
    GetGroupPhysicsEventsByID(EventGroups);

    // 2. BatchSyncPhysicsResults와 동일한 패턴: 한 번의 SoA 순회
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx i = startIdx; i < endIdx; ++i)
    {
        // 할당되고 활성화된 슬롯만 처리
        if (!PhysicsStateSoA->IsValidActiveSlotIndex(i))
            continue;

        PhysicsID currentPhysicsID = PhysicsStateSoA->GetID(i);

        // 해당 PhysicsID에 이벤트가 있는지 확인
        auto eventIt = EventGroups.find(currentPhysicsID);
        if (eventIt == EventGroups.end())
            continue;

        // IPhysicsObject에 원본 물리 이벤트 배치 전송
        if (auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock())
        {
            physicsObject->ReceiveCollisionEvents(eventIt->second);
        }
    }

    // 3. 이벤트 큐 정리
    ClearEventQueue();
}

void UPhysicsSystem::GetGroupPhysicsEventsByID(std::unordered_map<PhysicsID, std::vector<FPhysicsCollisionEvent>>& EventGroups)
{
    // 큐 크기 기반 메모리 예약으로 동적 할당 최소화
    EventGroups.reserve(CollisionEventQueue->Size() / 2); // 평균적으로 한 객체당 2개 이벤트 가정

    // FIFO 순서로 이벤트 처리 및 그룹화
    while (!CollisionEventQueue->Empty())
    {
        FPhysicsCollisionEvent physicsEvent = CollisionEventQueue->Front();
        CollisionEventQueue->Pop();

        // A에게 전송할 이벤트 (PhysicsIdB가 상대방)
        EventGroups[physicsEvent.PhysicsIdA].emplace_back(physicsEvent);

        // B에게 전송할 이벤트 (PhysicsIdA가 상대방)
        FPhysicsCollisionEvent reverseEvent = physicsEvent;
        reverseEvent.PhysicsIdA = physicsEvent.PhysicsIdB;
        reverseEvent.PhysicsIdB = physicsEvent.PhysicsIdA;
        EventGroups[physicsEvent.PhysicsIdB].emplace_back(reverseEvent);
    }
}

void UPhysicsSystem::ClearEventQueue()
{
    if (CollisionEventQueue)
    {
        CollisionEventQueue->Clear();
    }
}

size_t UPhysicsSystem::GetEventQueueSize() const
{
    return CollisionEventQueue ? CollisionEventQueue->Size() : 0;
}


#pragma endregion

#pragma region Job System Management

void UPhysicsSystem::ProcessJobQueue()
{
    // Job Queue를 순차적으로 처리
    while (!JobQueue->Empty())
    {
        auto request = JobQueue->Front();
        JobQueue->Pop();

        if (request.IsValid())
        {
            // Job 실행 - this를 IPhysicsStateInternal*로 전달
            request.Job->Execute(this);
        }
    }
}

#pragma endregion

#pragma region Physics Simulation Pipeline

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
    PhysicsStateSoA->CleanupExpiredObjectRefs();

    // 4. JobQueue 클리어
    JobQueue->Clear();

    // 5. JobPool 클리어
    JobPool->Reset();
}

// 단일 서브스텝 시뮬레이션
float UPhysicsSystem::SimulateSubstep(const float StepTime)
{
    //가장 적은 시뮬시간
    float MinSimulatedTimeRatio = 1.0f;

    // 1. 충돌 
    std::vector<PhysicsID> ActiveIDs = GetActivePhysicsIDs();
    float CollideTimeRatio = GetCollisionSubsystem()->ProcessCollisions(ActiveIDs, StepTime);
    MinSimulatedTimeRatio = std::min(MinSimulatedTimeRatio, CollideTimeRatio);

    // 시뮬레이션 시간 업데이트
    // Tick 시간 클램핑 ( 로직 처리에 안정성을 주기위한 최소 틱시간 결정)
    float SimualtedTime = std::max(MinSubStepTickTime, StepTime * MinSimulatedTimeRatio);


    //공통 물리 배치 시뮬레이션
    // 중력 적용
    BatchApplyGravity(Gravity, SimualtedTime);

    // 외부 힘 적용
    BatchApplyForces(SimualtedTime);

    // 드래그 적용
    BatchApplyDrag(SimualtedTime);

    // 속도 적분 (위치 업데이트)
    BatchIntegrateVelocity(SimualtedTime);

    //누적힘 리셋
    BatchResetForces();

    // 물리 Tick
    BatchPhysicsTick(SimualtedTime);

    // todo 비동기 이벤트 큐 푸시
    // TODO:

    return SimualtedTime;
}

void UPhysicsSystem::FinalizeSimulation()
{
    // 시뮬레이션 플래그 해제
    bIsSimulating = false;

    // 물리 → 게임 상태값 동기화 
    SyncPhysicsToGame();

    // 물리 이벤트 동기화
    SyncPhysicsEvents();
}

void UPhysicsSystem::BatchPhysicsTick(float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i) || PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // ObjectReferences를 통한 IPhysicsObject::TickPhysics 호출
            if (i < PhysicsStateSoA->ObjectReferences.size())
            {
                if (auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock())
                {
                    //physicsObject->TickPhysics(deltaTime);
                }
            }
        }
    }
}

void UPhysicsSystem::BatchApplyGravity(const Vector3& gravity, float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    // 중력은 이미 가속도(m/s²)이므로 deltaTime을 곱해서 속도 변화량으로 변환
    XMVECTOR gravityVec = XMVectorSet(gravity.x, gravity.y, gravity.z, 0.0f);
    XMVECTOR deltaTimeVec = XMVectorReplicate(deltaTime);

    // 올바른 계산: gravity(가속도) * deltaTime = 속도 변화량
    XMVECTOR gravityVelocityDelta = XMVectorMultiply(gravityVec, deltaTimeVec);

    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i) || PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // 중력 적용 조건 확인
            if (PhysicsStateSoA->PhysicsMasks[i].HasFlag(FPhysicsMask::MASK_GRAVITY_AFFECTED) &&
                PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Dynamic)
            {
                float invMass = PhysicsStateSoA->InvMasses[i];
                if (invMass <= KINDA_SMALL)  // Static 객체는 무한 질량
                    continue;

                // 중력 스케일 적용
                float gravityScale = PhysicsStateSoA->GravityScales[i];
                XMVECTOR scaledGravityDelta = XMVectorScale(gravityVelocityDelta, gravityScale);

                // 직접 속도에 변화량 적용 (질량은 이미 중력에 반영되어 있음)
                // 실제 물리에서는 모든 객체가 같은 중력 가속도를 받음
                XMVECTOR currentVelocity = PhysicsStateSoA->Velocities[i];
                XMVECTOR newVelocity = XMVectorAdd(currentVelocity, scaledGravityDelta);

                if (IsValidLinearVelocity(newVelocity))
                {
                    PhysicsStateSoA->Velocities[i] = newVelocity;
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
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i) || PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // 선형 속도 적분 + 속도 제한 (Position += Velocity * deltaTime)
            XMVECTOR velocity = PhysicsStateSoA->Velocities[i];
            if (IsValidLinearVelocity(velocity))
            {
                // 속도 제한 적용 (적분 전에)
                float maxSpeed = PhysicsStateSoA->MaxSpeeds[i];
                if (maxSpeed > -KINDA_SMALL)  // 0.0 포함
                {
                    ClampLinearVelocity(maxSpeed, velocity);
                    PhysicsStateSoA->Velocities[i] = velocity;  // 제한된 속도 저장
                }

                XMVECTOR currentPosition = PhysicsStateSoA->WorldPosition[i];
                XMVECTOR deltaPosition = XMVectorMultiply(velocity, deltaTimeVec);
                XMVECTOR newPosition = XMVectorAdd(currentPosition, deltaPosition);

                PhysicsStateSoA->WorldPosition[i] = newPosition;
            }

            // 각속도 적분 + 각속도 제한 (Rotation += AngularVelocity * deltaTime)
            XMVECTOR angularVelocity = PhysicsStateSoA->AngularVelocities[i];
            if (IsValidAngularVelocity(angularVelocity))
            {
                // 각속도 제한 적용 (적분 전에)
                float maxAngularSpeed = PhysicsStateSoA->MaxAngularSpeeds[i];
                if (maxAngularSpeed > 0.0f)  // 음수는 무제한
                {
                    ClampAngularVelocity(maxAngularSpeed, angularVelocity);
                    PhysicsStateSoA->AngularVelocities[i] = angularVelocity;  // 제한된 각속도 저장
                }

                XMVECTOR currentRotation = PhysicsStateSoA->WorldRotationQuat[i];

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

                    PhysicsStateSoA->WorldRotationQuat[i] = newRotation;
                }
            }
        }
    }
}

void UPhysicsSystem::BatchResetForces()
{
    XMVECTOR zeroVector = XMVectorZero();

    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당된 슬롯만 처리 (활성화 여부 무관하게 힘 초기화)
            if (!PhysicsStateSoA->IsValidSlotIndex(i) || PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            PhysicsStateSoA->AccumulatedForces[i] = zeroVector;
            PhysicsStateSoA->AccumulatedTorques[i] = zeroVector;
        }
    }
}

void UPhysicsSystem::BatchApplyForces(float deltaTime)
{
    if (deltaTime <= KINDA_SMALL)
        return;

    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();
    XMVECTOR deltaTimeVec = XMVectorReplicate(deltaTime);

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 유효성 및 시뮬레이션 대상 검증
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i) ||
                PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            float currentInvMass = PhysicsStateSoA->InvMasses[i];
            if (currentInvMass <= KINDA_SMALL)  // 무한 질량 객체 제외
                continue;


            // 선형 가속도
            XMVECTOR currentForce = PhysicsStateSoA->AccumulatedForces[i];
            XMVECTOR currentInvMassVec = XMVectorReplicate(currentInvMass);
            XMVECTOR currentLinearAcceleration = XMVectorMultiply(currentForce, currentInvMassVec);

            // 가속도 적분
            XMVECTOR deltaVelocity = XMVectorMultiply(currentLinearAcceleration, deltaTimeVec);
            XMVECTOR currentVelocity = PhysicsStateSoA->Velocities[i];
            XMVECTOR newVelocity = XMVectorAdd(currentVelocity, deltaVelocity);

            // 안전성 검증 후 적용
            if (IsValidLinearVelocity(newVelocity))
            {
                PhysicsStateSoA->Velocities[i] = newVelocity;
            }

            // 토크 각가속도
            XMVECTOR currentTorque = PhysicsStateSoA->AccumulatedTorques[i];
            XMVECTOR currentInvRotationalInertia = PhysicsStateSoA->InvRotationalInertias[i];
            XMVECTOR currentAngularAcceleration = XMVectorMultiply(currentTorque, currentInvRotationalInertia);

            // 각속도 전환
            XMVECTOR deltaAngularVelocity = XMVectorMultiply(currentAngularAcceleration, deltaTimeVec);
            XMVECTOR currentAngularVelocity = PhysicsStateSoA->AngularVelocities[i];
            XMVECTOR newAngularVelocity = XMVectorAdd(currentAngularVelocity, deltaAngularVelocity);

            // 안전성 검증 후 적용
            if (IsValidAngularVelocity(newAngularVelocity))
            {
                PhysicsStateSoA->AngularVelocities[i] = newAngularVelocity;
            }

            //// 힘과 토크 누적 초기화 (다음 프레임을 위한 준비)
            //PhysicsStateSoA->AccumulatedForces[i] = XMVectorZero();
            //PhysicsStateSoA->AccumulatedTorques[i] = XMVectorZero();
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

    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i) || PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Static)
                continue;

            // Dynamic 타입만 드래그 적용
            if (PhysicsStateSoA->PhysicsTypes[i] == EPhysicsType::Dynamic)
            {
                // 선형 속도 드래그 적용
                XMVECTOR currentVelocity = PhysicsStateSoA->Velocities[i];
                if (IsValidLinearVelocity(currentVelocity))
                {
                    // 속도가 매우 작으면 완전히 정지시켜 진동 방지
                    float velocityMagnitude = XMVector3Length(currentVelocity).m128_f32[0];
                    if (velocityMagnitude < 0.01f)
                    {
                        PhysicsStateSoA->Velocities[i] = XMVectorZero();
                    }
                    else
                    {
                        XMVECTOR newVelocity = XMVectorMultiply(currentVelocity, linearDragVec);
                        PhysicsStateSoA->Velocities[i] = newVelocity;
                    }
                }

                // 각속도 드래그 적용
                XMVECTOR currentAngularVel = PhysicsStateSoA->AngularVelocities[i];
                if (IsValidAngularVelocity(currentAngularVel))
                {
                    // 각속도가 매우 작으면 완전히 정지시켜 진동 방지
                    float angularMagnitude = XMVector3Length(currentAngularVel).m128_f32[0];
                    if (angularMagnitude < 0.1f)  // 약 5.7도/초 이하면 정지
                    {
                        PhysicsStateSoA->AngularVelocities[i] = XMVectorZero();
                    }
                    else
                    {
                        XMVECTOR newAngularVel = XMVectorMultiply(currentAngularVel, angularDragVec);
                        PhysicsStateSoA->AngularVelocities[i] = newAngularVel;
                    }
                }
            }
        }
    }
}

bool UPhysicsSystem::IsValidLinearVelocity(const XMVECTOR& InVelocity)
{
    // 속도 크기 계산
    float magnitude = XMVector3Length(InVelocity).m128_f32[0];
    // 최대 허용 속도 검사 (물리적으로 합리적인 범위)
    return magnitude > KINDA_SMALL && magnitude < MaxPhysicsVelocity;
}

bool UPhysicsSystem::IsValidAngularVelocity(const XMVECTOR& InAngularVelocity)
{
    // 각속도 크기 계산
    float magnitude = XMVector3Length(InAngularVelocity).m128_f32[0];
    // 최대 허용 각속도 검사 (라디안/초)
    return magnitude > KINDA_SMALL && magnitude < MaxPhysicsAngularVelocity;
}

bool UPhysicsSystem::IsValidForce(const XMVECTOR& InForce)
{
    // 힘의 크기 계산
    float magnitude = XMVector3Length(InForce).m128_f32[0];
    // 최대 허용 힘 검사 (뉴턴)
    return magnitude > KINDA_SMALL && magnitude < MaxPhysicsForce;
}

bool UPhysicsSystem::IsValidTorque(const XMVECTOR& InTorque)
{
    // 토크의 크기 계산
    float magnitude = XMVector3Length(InTorque).m128_f32[0];
    // 최대 허용 토크 검사 (뉴턴·미터)
    return magnitude > KINDA_SMALL && magnitude < MaxPhysicsTorque;
}

bool UPhysicsSystem::IsValidLinearAcceleration(const XMVECTOR& InAccel)
{
    // 가속도 크기 계산
    float magnitude = XMVector3Length(InAccel).m128_f32[0];
    // 최대 허용 가속도 검사 (m/s²)
    return magnitude > KINDA_SMALL && magnitude < MaxPhysicsAcceleration;
}

bool UPhysicsSystem::IsValidAngularAcceleration(const XMVECTOR& InAngularAccel)
{
    // 각가속도 크기 계산
    float magnitude = XMVector3Length(InAngularAccel).m128_f32[0];
    // 최대 허용 각가속도 검사 (라디안/초²)
    return magnitude > KINDA_SMALL && magnitude < MaxPhysicsAngularAcceleration;
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

#pragma region Synchronization System

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
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // High Frequency 더티 플래그 확인
            FPhysicsDataDirtyFlags dirtyFlags = physicsObject->GetDirtyFlags();
            if (!dirtyFlags.HasHighFreq())
                continue;

            // Transform 데이터 동기화
            FHighFrequencyData data = physicsObject->GetHighFrequencyData();

            // === 현재 프레임 Transform 동기화 ===
            PhysicsStateSoA->WorldPosition[i] = XMVectorSet(
                data.Position.x, data.Position.y, data.Position.z, 1.0f);
            PhysicsStateSoA->WorldRotationQuat[i] = XMVectorSet(
                data.Rotation.x, data.Rotation.y, data.Rotation.z, data.Rotation.w);
            PhysicsStateSoA->WorldScale[i] = XMVectorSet(
                data.Scale.x, data.Scale.y, data.Scale.z, 1.0f);

            // === 이전 프레임 Transform 동기화 (CCD용) ===
            PhysicsStateSoA->PrevWorldPosition[i] = XMVectorSet(
                data.PrevPosition.x, data.PrevPosition.y, data.PrevPosition.z, 1.0f);
            PhysicsStateSoA->PrevWorldRotationQuat[i] = XMVectorSet(
                data.PrevRotation.x, data.PrevRotation.y, data.PrevRotation.z, data.PrevRotation.w);

            //// PrevWorldScale은 사용하지 않음
            //PhysicsStateSoA->PrevWorldScale[i] = XMVectorSet(
            //    data.Scale.x, data.Scale.y, data.Scale.z, 1.0f);
        }
    }
}

void UPhysicsSystem::BatchSyncMidFrequencyData()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // Mid Frequency 더티 플래그 확인
            FPhysicsDataDirtyFlags dirtyFlags = physicsObject->GetDirtyFlags();
            if (!dirtyFlags.HasMidFreq())
                continue;

            // Type, Mask 데이터 동기화
            FMidFrequencyData data = physicsObject->GetMidFrequencyData();

            PhysicsStateSoA->PhysicsTypes[i] = data.PhysicsType;
            PhysicsStateSoA->PhysicsMasks[i] = data.PhysicsMask;
        }
    }
}

void UPhysicsSystem::BatchSyncLowFrequencyData()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // Low Frequency 더티 플래그 확인
            FPhysicsDataDirtyFlags dirtyFlags = physicsObject->GetDirtyFlags();
            if (!dirtyFlags.HasLowFreq())
                continue;

            // Properties 데이터 동기화
            FLowFrequencyData data = physicsObject->GetLowFrequencyData();

            PhysicsStateSoA->InvMasses[i] = data.InvMass;
            PhysicsStateSoA->FrictionKinetics[i] = data.FrictionKinetic;
            PhysicsStateSoA->FrictionStatics[i] = data.FrictionStatic;
            PhysicsStateSoA->Restitutions[i] = data.Restitution;
            PhysicsStateSoA->InvRotationalInertias[i] = XMVectorSet(
                data.InvRotationalInertia.x,
                data.InvRotationalInertia.y,
                data.InvRotationalInertia.z,
                0.0f);
            PhysicsStateSoA->MaxSpeeds[i] = data.MaxSpeed;
            PhysicsStateSoA->MaxAngularSpeeds[i] = data.MaxAngularSpeed;
            PhysicsStateSoA->GravityScales[i] = data.GravityScale;
			PhysicsStateSoA->CollisionShapeTypes[i] = data.CollisionShapeType;
            PhysicsStateSoA->CollisionWorldHalfExtents[i] = XMVectorSet(
                data.CollisionWorldHalfExtent.x,
                data.CollisionWorldHalfExtent.y,
                data.CollisionWorldHalfExtent.z,
				0.0f);
        }
    }
}

void UPhysicsSystem::BatchSyncPhysicsResults()
{
    // Loop tiling을 이용한 캐시 최적화 순회
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당되고 활성화된 슬롯만 처리
            if (!PhysicsStateSoA->IsValidActiveSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // 물리 시뮬레이션 결과 데이터 구성
            FPhysicsToGameData physicsResults;

            // SIMD 최적화된 데이터 읽기
            XMVECTOR velocity = PhysicsStateSoA->Velocities[i];
            XMVECTOR angularVelocity = PhysicsStateSoA->AngularVelocities[i];
            XMVECTOR position = PhysicsStateSoA->WorldPosition[i];
            XMVECTOR rotation = PhysicsStateSoA->WorldRotationQuat[i];
            XMVECTOR scale = PhysicsStateSoA->WorldScale[i];

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
    const SoAIdx startIdx = PhysicsStateSoA->GetStartIdx();
    const SoAIdx endIdx = PhysicsStateSoA->GetEndIdx();

    for (SoAIdx batchStart = startIdx; batchStart < endIdx; batchStart += BatchSize)
    {
        SoAIdx batchEnd = std::min(batchStart + BatchSize, endIdx);

        for (SoAIdx i = batchStart; i < batchEnd; ++i)
        {
            // 할당된 슬롯만 처리 (활성화 여부 무관하게 더티 플래그 정리)
            if (!PhysicsStateSoA->IsValidSlotIndex(i))
                continue;

            auto physicsObject = PhysicsStateSoA->ObjectReferences[i].lock();
            if (!physicsObject)
                continue;

            // 모든 더티 플래그 일괄 정리
            FPhysicsDataDirtyFlags allFlags(FPhysicsDataDirtyFlags::FLAG_ALL);
            physicsObject->MarkDataClean(allFlags);
        }
    }
}

#pragma endregion

#pragma region IPhysicsStateInternal Implementation

// === Physical Properties Access ===

float UPhysicsSystem::P_GetMass(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    float invMass = PhysicsStateSoA->InvMasses[index];
    return (invMass > KINDA_SMALL) ? (1.0f / invMass) : 0.0f;
}

float UPhysicsSystem::P_GetInvMass(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->InvMasses[index];
}

XMVECTOR UPhysicsSystem::P_GetRotationalInertia(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    XMVECTOR invInertia = PhysicsStateSoA->InvRotationalInertias[index];

    // 역관성에서 관성으로 변환 (각 성분별로)
    XMVECTOR inertia = XMVectorReciprocal(invInertia);

    // 매우 작은 역관성(무한 관성) 처리
    XMVECTOR mask = XMVectorGreater(invInertia, XMVectorReplicate(KINDA_SMALL));
    XMVECTOR clampedInertia = XMVectorMin(inertia, XMVectorReplicate(KINDA_LARGE));

    return XMVectorSelect(XMVectorReplicate(KINDA_LARGE), clampedInertia, mask);
}

XMVECTOR UPhysicsSystem::P_GetInvRotationalInertia(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->InvRotationalInertias[index];
}

float UPhysicsSystem::P_GetRestitution(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->Restitutions[index];
}

float UPhysicsSystem::P_GetFrictionStatic(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->FrictionStatics[index];
}

float UPhysicsSystem::P_GetFrictionKinetic(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->FrictionKinetics[index];
}

float UPhysicsSystem::P_GetGravityScale(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->GravityScales[index];
}

float UPhysicsSystem::P_GetMaxSpeed(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->MaxSpeeds[index];
}

float UPhysicsSystem::P_GetMaxAngularSpeed(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return 0.0f;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->MaxAngularSpeeds[index];
}

// === Motion State Access ===

XMVECTOR UPhysicsSystem::P_GetVelocity(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->Velocities[index];
}

XMVECTOR UPhysicsSystem::P_GetAngularVelocity(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->AngularVelocities[index];
}

XMVECTOR UPhysicsSystem::P_GetAccumulatedForce(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->AccumulatedForces[index];
}

XMVECTOR UPhysicsSystem::P_GetAccumulatedTorque(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->AccumulatedTorques[index];
}

// === Transform Access ===

XMVECTOR UPhysicsSystem::P_GetWorldPosition(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->WorldPosition[index];
}

XMVECTOR UPhysicsSystem::P_GetWorldRotationQuat(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMQuaternionIdentity();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->WorldRotationQuat[index];
}

XMVECTOR UPhysicsSystem::P_GetWorldScale(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMVectorSplatOne();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->WorldScale[index];
}

XMMATRIX UPhysicsSystem::P_GetWorldTransformMatrix(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMMatrixIdentity();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    XMVECTOR position = PhysicsStateSoA->WorldPosition[index];
    XMVECTOR rotation = PhysicsStateSoA->WorldRotationQuat[index];
    XMVECTOR scale = PhysicsStateSoA->WorldScale[index];

    return XMMatrixAffineTransformation(scale, XMVectorZero(), rotation, position);
}

XMVECTOR UPhysicsSystem::P_GetPrevWorldPosition(PhysicsID id) const
{
    if (!IsValidTargetID(id))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(id));
    return PhysicsStateSoA->PrevWorldPosition[index];
}

XMVECTOR UPhysicsSystem::P_GetPrevWorldRotationQuat(PhysicsID id) const
{
    if (!IsValidTargetID(id))
        return XMQuaternionIdentity();

    SoAIdx index = GetIdx(static_cast<SoAID>(id));
    return PhysicsStateSoA->PrevWorldRotationQuat[index];
}

XMVECTOR UPhysicsSystem::P_GetPrevWorldScale(PhysicsID id) const
{
    if (!IsValidTargetID(id))
        return XMVectorSplatOne();

    SoAIdx index = GetIdx(static_cast<SoAID>(id));
    return PhysicsStateSoA->PrevWorldScale[index];
}

XMMATRIX UPhysicsSystem::P_GetPrevWorldTransformMatrix(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return XMMatrixIdentity();

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    XMVECTOR position = PhysicsStateSoA->PrevWorldPosition[index];
    XMVECTOR rotation = PhysicsStateSoA->PrevWorldRotationQuat[index];
    XMVECTOR scale = PhysicsStateSoA->PrevWorldScale[index];

    return XMMatrixAffineTransformation(scale, XMVectorZero(), rotation, position);
}

// === State Type and Control ===

bool UPhysicsSystem::P_IsPhysicsActive(PhysicsID targetID) const
{
    if (!IsValidTargetID(targetID))
        return false;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    return PhysicsStateSoA->PhysicsMasks[index].HasFlag(FPhysicsMask::MASK_ACTIVATION);
}

// === Force and Impulse Application ===

void UPhysicsSystem::P_ApplyForce(PhysicsID targetID, XMVECTOR force, XMVECTOR location)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_ApplyForce: %u", targetID);
        return;
    }

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    // Static 타입 체크
    if (PhysicsStateSoA->PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_ApplyForce blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    if (!IsValidForce(force))
    {
        LOG_WARNING("Invalid force value for PhysicsID %u", targetID);
        return;
    }

    // 중심에서 적용점까지의 벡터
    XMVECTOR centerOfMass = PhysicsStateSoA->WorldPosition[index];
    XMVECTOR radius = XMVectorSubtract(location, centerOfMass);

    // 힘을 누적 힘에 추가
    XMVECTOR currentForce = PhysicsStateSoA->AccumulatedForces[index];
    PhysicsStateSoA->AccumulatedForces[index] = XMVectorAdd(currentForce, force);

    // 토크 계산 및 추가 (radius × force)
    XMVECTOR torque = XMVector3Cross(radius, force);

    if (IsValidTorque(torque))
    {
        XMVECTOR currentTorque = PhysicsStateSoA->AccumulatedTorques[index];
        PhysicsStateSoA->AccumulatedTorques[index] = XMVectorAdd(currentTorque, torque);
    }
}

void UPhysicsSystem::P_ApplyImpulse(PhysicsID targetID, XMVECTOR impulse, XMVECTOR location)
{
    if (!IsValidTargetID(targetID))
    {
        LOG_ERROR("Invalid PhysicsID for P_ApplyImpulse: %u", targetID);
        return;
    }

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    // Static 타입 체크
    if (PhysicsStateSoA->PhysicsTypes[index] == EPhysicsType::Static)
    {
        LOG_WARNING("P_ApplyImpulse blocked: PhysicsID %u is Static type", targetID);
        return;
    }

    if (!IsValidForce(impulse))
    {
        LOG_WARNING("Invalid impulse value for PhysicsID %u", targetID);
        return;
    }

    // 선형 충격 적용
    float invMass = PhysicsStateSoA->InvMasses[index];
    XMVECTOR deltaVelocity = XMVectorScale(impulse, invMass);

    XMVECTOR currentVelocity = PhysicsStateSoA->Velocities[index];
    XMVECTOR newVelocity = XMVectorAdd(currentVelocity, deltaVelocity);

    if (IsValidLinearVelocity(newVelocity))
    {
        PhysicsStateSoA->Velocities[index] = newVelocity;
    }

    // 각속도 충격 적용
    XMVECTOR centerOfMass = PhysicsStateSoA->WorldPosition[index];
    XMVECTOR radius = XMVectorSubtract(location, centerOfMass);
    XMVECTOR angularImpulse = XMVector3Cross(radius, impulse);

    XMVECTOR invInertia = PhysicsStateSoA->InvRotationalInertias[index];
    XMVECTOR deltaAngularVelocity = XMVectorMultiply(angularImpulse, invInertia);

    XMVECTOR currentAngularVelocity = PhysicsStateSoA->AngularVelocities[index];
    XMVECTOR newAngularVelocity = XMVectorAdd(currentAngularVelocity, deltaAngularVelocity);

    if (IsValidAngularVelocity(newAngularVelocity))
    {
        PhysicsStateSoA->AngularVelocities[index] = newAngularVelocity;
    }
}

// === Property Setters ===

void UPhysicsSystem::P_SetMass(PhysicsID targetID, float mass)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    if (mass > KINDA_SMALL)
    {
        PhysicsStateSoA->InvMasses[index] = 1.0f / mass;
    }
    else
    {
        PhysicsStateSoA->InvMasses[index] = KINDA_SMALL; // 무한 질량 (매우 작은 역질량)
    }
}

void UPhysicsSystem::P_SetInvMass(PhysicsID targetID, float invMass)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    // 매우 큰 역질량 제한
    float clampedInvMass = std::clamp(invMass, KINDA_SMALL, KINDA_LARGE);
    PhysicsStateSoA->InvMasses[index] = clampedInvMass;
}

void UPhysicsSystem::P_SetRotationalInertia(PhysicsID targetID, XMVECTOR rotationalInertia)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    // 관성에서 역관성으로 변환 (각 성분별로)
    XMVECTOR invInertia = XMVectorReciprocal(rotationalInertia);

    // 매우 작은 관성(무한 관성) 처리
    XMVECTOR mask = XMVectorGreater(rotationalInertia, XMVectorReplicate(KINDA_SMALL));
    XMVECTOR clampedInvInertia = XMVectorMin(invInertia, XMVectorReplicate(KINDA_LARGE));
    XMVECTOR finalInvInertia = XMVectorSelect(XMVectorReplicate(KINDA_SMALL), clampedInvInertia, mask);

    PhysicsStateSoA->InvRotationalInertias[index] = finalInvInertia;
}

void UPhysicsSystem::P_SetInvRotationalInertia(PhysicsID targetID, XMVECTOR invRotationalInertia)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    // 역관성 범위 제한 (KINDA_SMALL ~ KINDA_LARGE)
    XMVECTOR minInvInertia = XMVectorReplicate(KINDA_SMALL);
    XMVECTOR maxInvInertia = XMVectorReplicate(KINDA_LARGE);
    XMVECTOR clampedInvInertia = XMVectorClamp(invRotationalInertia, minInvInertia, maxInvInertia);

    PhysicsStateSoA->InvRotationalInertias[index] = clampedInvInertia;
}

void UPhysicsSystem::P_SetRestitution(PhysicsID targetID, float restitution)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->Restitutions[index] = std::clamp(restitution, 0.0f, 1.0f);
}

void UPhysicsSystem::P_SetFrictionStatic(PhysicsID targetID, float frictionStatic)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->FrictionStatics[index] = std::max(0.0f, frictionStatic);
}

void UPhysicsSystem::P_SetFrictionKinetic(PhysicsID targetID, float frictionKinetic)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->FrictionKinetics[index] = std::max(0.0f, frictionKinetic);
}

void UPhysicsSystem::P_SetGravityScale(PhysicsID targetID, float gravityScale)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->GravityScales[index] = gravityScale;
}

void UPhysicsSystem::P_SetMaxSpeed(PhysicsID targetID, float maxSpeed)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->MaxSpeeds[index] = std::max(0.0f, maxSpeed);
}

void UPhysicsSystem::P_SetMaxAngularSpeed(PhysicsID targetID, float maxAngularSpeed)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->MaxAngularSpeeds[index] = std::max(0.0f, maxAngularSpeed);
}

void UPhysicsSystem::P_SetVelocity(PhysicsID targetID, const XMVECTOR& velocity)
{
    if (!IsValidTargetID(targetID))
        return;
    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->Velocities[index] = velocity;
}

void UPhysicsSystem::P_AddVelocity(PhysicsID targetID, const XMVECTOR& deltaVelocity)
{
}

void UPhysicsSystem::P_SetAngularVelocity(PhysicsID targetID, const XMVECTOR& Angularvelocity)
{
}

void UPhysicsSystem::P_AddAngularVelocity(PhysicsID targetID, const XMVECTOR& deltaAngularVelocity)
{
}

// === Transform Setters ===

void UPhysicsSystem::P_SetWorldPosition(PhysicsID targetID, XMVECTOR worldPosition)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->WorldPosition[index] = worldPosition;
}

void UPhysicsSystem::P_SetWorldRotation(PhysicsID targetID, XMVECTOR worldRotation)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->WorldRotationQuat[index] = worldRotation;
}

void UPhysicsSystem::P_SetWorldScale(PhysicsID targetID, XMVECTOR worldScale)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->WorldScale[index] = worldScale;
}

void UPhysicsSystem::P_SetPrevWorldPosition(PhysicsID targetID, XMVECTOR worldPosition)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->PrevWorldPosition[index] = worldPosition;
}

void UPhysicsSystem::P_SetPrevWorldRotation(PhysicsID targetID, XMVECTOR worldRotation)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->PrevWorldRotationQuat[index] = worldRotation;
}

void UPhysicsSystem::P_SetPrevWorldScale(PhysicsID targetID, XMVECTOR worldScale)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->PrevWorldScale[index] = worldScale;
}

// === State Type and Control ===

void UPhysicsSystem::P_SetPhysicsType(PhysicsID targetID, EPhysicsType physicsType)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->PhysicsTypes[index] = physicsType;
}

void UPhysicsSystem::P_SetPhysicsMask(PhysicsID targetID, const FPhysicsMask& physicsMask)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));
    PhysicsStateSoA->PhysicsMasks[index] = physicsMask;
}

void UPhysicsSystem::P_SetPhysicsActive(PhysicsID targetID, bool bActive)
{
    if (!IsValidTargetID(targetID))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(targetID));

    if (bActive)
    {
        PhysicsStateSoA->PhysicsMasks[index].SetFlag(FPhysicsMask::MASK_ACTIVATION);
    }
    else
    {
        PhysicsStateSoA->PhysicsMasks[index].ClearFlag(FPhysicsMask::MASK_ACTIVATION);
    }
}

#pragma endregion

#pragma region ICollisionShapeInternal Implementation

ECollisionShapeType UPhysicsSystem::P_GetShapeType(PhysicsID id) const
{
    if (!IsValidTargetID(id))
        return ECollisionShapeType::None;

    SoAIdx index = GetIdx(static_cast<SoAID>(id));
    return PhysicsStateSoA->CollisionShapeTypes[index];
}

void UPhysicsSystem::P_SetShapeType(PhysicsID id, ECollisionShapeType type)
{
    if (!IsValidTargetID(id))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(id));
    PhysicsStateSoA->CollisionShapeTypes[index] = type;
}

XMVECTOR UPhysicsSystem::P_GetShapeHalfExtent(PhysicsID id) const
{
    if (!IsValidTargetID(id))
        return XMVectorZero();

    SoAIdx index = GetIdx(static_cast<SoAID>(id));
    return PhysicsStateSoA->CollisionWorldHalfExtents[index];
}

void UPhysicsSystem::P_SetShapeHalfExtent(PhysicsID id, XMVECTOR extent)
{
    if (!IsValidTargetID(id))
        return;

    SoAIdx index = GetIdx(static_cast<SoAID>(id));

    // 음수 값 방지 (절댓값 적용)
    XMVECTOR validExtent = XMVectorAbs(extent);
    PhysicsStateSoA->CollisionWorldHalfExtents[index] = validExtent;
}

#pragma endregion