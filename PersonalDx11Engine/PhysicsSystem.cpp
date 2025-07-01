#include "PhysicsSystem.h"
#include "CollisionProcessor.h"
#include <algorithm>
#include "Debug.h"
#include "ConfigReadManager.h"

UPhysicsSystem::UPhysicsSystem()
{
}

UPhysicsSystem::~UPhysicsSystem()
{
    Release();
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

void UPhysicsSystem::RequestPhysicsJob(const FPhysicsJobRequest& RequestedJob)
{
    if (!RequestedJob.IsValid())
    {
        LOG_FUNC_CALL("[WARNING] Invalid Job Requested!");
        return;
    }
    JobQueue.Push(RequestedJob);
}

void UPhysicsSystem::RegisterPhysicsObject(std::shared_ptr<IPhysicsObject>& InObject)
{
    if (!InObject)
        return;

    auto It = std::find_if(RegisteredObjects.begin(), RegisteredObjects.end(), [&InObject](const std::weak_ptr<IPhysicsObject>& Object)
                 {
                     return Object.lock() == InObject;
                 });
    //이미 등록된 객체
    if (It != RegisteredObjects.end())
        return;
    
    RegisteredObjects.push_back(InObject);
    PhysicsID NewPhysicsID = PhysicsStateSoA->AllocateSlot();
    InObject->SetPhysicsID(NewPhysicsID);
}

void UPhysicsSystem::UnregisterPhysicsObject(std::shared_ptr<IPhysicsObject>& InObject)
{
    if (!InObject)
        return;

    auto EraseIt = std::remove_if(
        RegisteredObjects.begin(), RegisteredObjects.end(),
        [&InObject](const std::weak_ptr<IPhysicsObject>& Object) {
            return Object.lock() == InObject;
        }
    );
    if (EraseIt != RegisteredObjects.end())
    {
        PhysicsID ErasedID = InObject->GetPhysicsID();
        RegisteredObjects.erase(EraseIt, RegisteredObjects.end());
        PhysicsStateSoA->DeallocateSlot(ErasedID);
    }
}

// 메인 물리 업데이트 (메인 루프에서 호출)
void UPhysicsSystem::TickPhysics(const float DeltaTime)
{
    // 시간 누적 및 서브스텝 계산
    AccumulatedTime += DeltaTime;
    int NumSubsteps = CalculateRequiredSubsteps();
    NumSubsteps = Math::Clamp(NumSubsteps, 0, MaxSubSteps);

    // 물리 시뮬레이션 준비
    PrepareSimulation();

    // 서브스텝 시뮬레이션
    for (int i = 0; i < MaxSubSteps; i++)
    {
        if (SimulateSubstep(FixedTimeStep) && i > MinSubSteps)
        {
            // 시간 전부 사용- 서브  스텝 종료
            break;
        }
    }

    // 시뮬레이션 결과 적용
    FinalizeSimulation();
}

void UPhysicsSystem::Initialzie()
{
    try
    {
        LoadConfigFromIni();
        RegisteredObjects.reserve(InitialPhysicsObjectCapacity);
        PhysicsStateSoA = std::make_unique<FPhysicsStateArrays>(InitialPhysicsObjectCapacity , 64, 0.332f);
        PhysicsJobPool.Initialize(InitialPhysicsJobPoolSizeMB * 1024 * 1024);
    }
    catch (...)
    {
        Release();
        exit(1);
    }
}

void UPhysicsSystem::Release()
{
    RegisteredObjects.clear();
    PhysicsJobPool.Reset();
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

    // 파괴된(만료된) 객체 소멸
    RegisteredObjects.erase(
        std::remove_if(
            RegisteredObjects.begin(),
            RegisteredObjects.end(),
            [](const std::weak_ptr<IPhysicsObject>& objWeak) {
                return objWeak.expired();
            }
        ),        RegisteredObjects.end()
    );

    // 작업 큐 차례대로 실행
    for (auto& RequestedJob : JobQueue)
    {
        if (!RequestedJob.IsValid())
            continue;
        RequestedJob.PhysicsJob->Execute(RequestedJob.TargetWeak.lock().get(), _placeholder_);
    }
    //JobQeueu 클리어
    JobQueue.Clear();
    //JobPool 클리어
    PhysicsJobPool.Reset();
}

// 단일 서브스텝 시뮬레이션
bool UPhysicsSystem::SimulateSubstep(const float StepTime)
{
	// 이미 충돌로 인해 전체 시간이 소진된 경우 검사
	if (AccumulatedTime < KINDA_SMALL)
	{
		return true; // 더 이상 시뮬레이션 불필요

	}
    //가장 적은 시뮬시간
    float MinSimulatedTimeRatio = 1.0f;
    
    // 1. 충돌 
    float CollideTimeRatio = GetCollisionSubsystem()->SimulateCollision(StepTime);
    MinSimulatedTimeRatio = std::min(MinSimulatedTimeRatio, CollideTimeRatio);

    // 시뮬레이션 시간 업데이트
    // Tick 시간 클램핑 ( 로직 처리에 안정성을 주기위한 최소 틱시간 결정)
    float RemainingTime = StepTime;
    float SimualtedTime = std::max(MinSubStepTickTime, StepTime * MinSimulatedTimeRatio);
    AccumulatedTime -= SimualtedTime;
    RemainingTime -= SimualtedTime;

    // 물리 Tick
    for (auto& PhysicsObject : RegisteredObjects)
    {
        //PhysicsTick 
        auto PhysicsObjectPtr = PhysicsObject.lock();
        if (PhysicsObjectPtr && SimualtedTime > KINDA_SMALL)
        {
            PhysicsObjectPtr->TickPhysics(SimualtedTime);
        }
       
    }

    if (RemainingTime < KINDA_SMALL)
    {
        //시간 전부 사용
        return true;

    }
    else
    {
        //시간 남음
        return false;
    }
}


// 시뮬레이션 완료 후 상태 적용
void UPhysicsSystem::FinalizeSimulation()
{
    bIsSimulating = false;

    // 모든 물리 객체에 최종 상태 동기화
    for (auto& ObjectWeak : RegisteredObjects)
    {
        auto Object = ObjectWeak.lock();
        if (!Object)
        {
            LOG_FUNC_CALL("[Error] InValid PhysicsObejct");
            continue;
        }

        if (Object->IsActive())
        {
            //시뮬레이션 결과 외부에 반영
            Object->SynchronizeCachedStateFromSimulated();
        }
    }
}

#pragma region Numeric Stability for PhysicsStates
bool UPhysicsSystem::IsValidForce(const Vector3& InForce)
{
    return InForce.LengthSquared() > 100.0f;
}

bool UPhysicsSystem::IsValidTorque(const Vector3& InTorque)
{
    return InTorque.LengthSquared() > 100.0f;
}

bool UPhysicsSystem::IsValidVelocity(const Vector3& InVelocity)
{
    return InVelocity.LengthSquared() > KINDA_SMALL;
}

bool UPhysicsSystem::IsValidAngularVelocity(const Vector3& InAngularVelocity)
{
    return InAngularVelocity.LengthSquared() > KINDA_SMALL;
}
bool UPhysicsSystem::IsValidAcceleration(const Vector3& InAccel)
{
    return InAccel.LengthSquared() > 1.0f;
}
bool UPhysicsSystem::IsValidAngularAcceleration(const Vector3& InAngularAccel)
{
    return InAngularAccel.LengthSquared() > 1.0f;;
}
#pragma endregion

void UPhysicsSystem::ClampVelocities(SoAID ObjectID)
{
    if (!PhysicsStateSoA->IsAllocatedSlot(ObjectID) ||
        !PhysicsStateSoA->IsActiveObject(ObjectID))
    {
        return;
    }

    SoAIdx Index = PhysicsStateSoA->GetIndex(ObjectID);

    XMVECTOR& Velocity = PhysicsStateSoA->Velocities[Index];
    const float MaxLinearSpeed = PhysicsStateSoA->MaxSpeeds[Index];

    XMVECTOR& AngularVelocity = PhysicsStateSoA->AngularVelocities[Index];
    const float MaxAngularSpeed = PhysicsStateSoA->MaxAngularSpeeds[Index];

    ClampLinearVelocity(MaxLinearSpeed, Velocity);
    ClampAngularVelocity(MaxAngularSpeed, AngularVelocity);
}

void UPhysicsSystem::ClampLinearVelocity(float InMaxSpeed, XMVECTOR& InOutVelocity)
{
    float Speed = XMVector3Length(InOutVelocity).m128_f32[0];

    if (Speed > InMaxSpeed)
    {
        InOutVelocity = XMVector3Normalize(InOutVelocity) * InMaxSpeed;
    }
    else if (Speed < KINDA_SMALL)
    {
        InOutVelocity = XMVectorZero();
    }
}

void UPhysicsSystem::ClampAngularVelocity(float InAngularMaxSpeed, XMVECTOR& InOutAngularVelocity)
{
    // 각속도 제한
    float angularSpeedSq = XMVector3Length(InOutAngularVelocity).m128_f32[0];

    if (angularSpeedSq > InAngularMaxSpeed * InAngularMaxSpeed)
    {
        InOutAngularVelocity = XMVector3Normalize(InOutAngularVelocity) * InAngularMaxSpeed;
    }
    else if (angularSpeedSq < KINDA_SMALL)
    {
        InOutAngularVelocity = XMVectorZero();
    }
}

///////////////////////////////////////////////////////////

void UPhysicsSystem::PrintDebugInfo()
{
#ifdef _DEBUG
    LOG("Current Active PhysicsObejct : [%03d]", RegisteredObjects.size());
#endif
}
