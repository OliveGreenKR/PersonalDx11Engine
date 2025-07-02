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
    //for (auto& PhysicsObject : RegisteredObjects)
    //{
    //    //PhysicsTick 
    //    auto PhysicsObjectPtr = PhysicsObject.lock();
    //    if (PhysicsObjectPtr && SimualtedTime > KINDA_SMALL)
    //    {
    //        PhysicsObjectPtr->TickPhysics(SimualtedTime);
    //    }
    //   
    //}

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


SoAIdx UPhysicsSystem::GetIdx(const SoAID targetID) const
{
    return PhysicsStateSoA.GetIndex(targetID);
}

bool UPhysicsSystem::IsValidTargetID(const SoAID targetID) const
{
    return PhysicsStateSoA.IsValidId(targetID);
}

///////////////////////////////////////////////////////////

void UPhysicsSystem::PrintDebugInfo()
{
#ifdef _DEBUG
    LOG_NORMAL("Current Active PhysicsObejct : [%03d]", PhysicsStateSoA.GetActiveObjectCount());
#endif
}
