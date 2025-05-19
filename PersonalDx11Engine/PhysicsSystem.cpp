#include "PhysicsSystem.h"
#include "CollisionProcessor.h"
#include <algorithm>
#include "Debug.h"

UPhysicsSystem::UPhysicsSystem()
{
    CollisionProcessor = new FCollisionProcessorT();
    assert(CollisionProcessor, "Allocate Failed");

}

UPhysicsSystem::~UPhysicsSystem()
{

    RegisteredObjects.clear();

    if (CollisionProcessor)
    {
        delete CollisionProcessor;
    }
}

// 메인 물리 업데이트 (게임 루프에서 호출)
void UPhysicsSystem::TickPhysics(const float DeltaTime)
{
    // 시간 누적 및 서브스텝 계산
    AccumulatedTime += DeltaTime;
    int NumSubsteps = CalculateRequiredSubsteps();
    NumSubsteps = Math::Clamp(NumSubsteps, 0, MaxSubsteps);

    // 물리 시뮬레이션 준비
    PrepareSimulation();

    // 서브스텝 시뮬레이션
    for (int i = 0; i < MaxSubsteps; i++)
    {
        if (SimulateSubstep(FixedTimeStep))
        {
            break;
        }
    }

    // 시뮬레이션 결과 적용
    FinalizeSimulation();
}

// 필요한 서브스텝 수 계산
int UPhysicsSystem::CalculateRequiredSubsteps()
{
    int steps = static_cast<int>(AccumulatedTime / FixedTimeStep);
    return std::min(steps, MaxSubsteps);
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
            [](const std::weak_ptr<IPhysicsObejct>& objWeak) {
                return objWeak.expired();
            }
        ),
        RegisteredObjects.end()
    );

    // 모든 물리 객체의 현재 상태 캡처
    for (auto& ObjectWeak : RegisteredObjects)
    {
        auto Object = ObjectWeak.lock();
        if (!Object)
        {
            LOG_FUNC_CALL("[Error] InValid PhysicsObejct");
            continue;
        }
   
        // 상태 저장 최적화 (변경된 객체만)
        if (Object->IsActive() && (Object->IsDirty()))
        {
            Object->CaptureState();
        }
    }

}

// 단일 서브스텝 시뮬레이션
bool UPhysicsSystem::SimulateSubstep(const float StepTime)
{
	// 이미 충돌로 인해 전체 시간이 소진된 경우 검사
	if (AccumulatedTime < KINDA_SMALL)
	{
		return true; // 더 이상 시뮬레이션 불필요

	}
    float ReamainingTimeRatio = 1.0f;
    float ConsumimgTimeRatio = 1.0f;

    // 충돌 처리
    //이때 CollisonManager는 캡처된 상태값을 이용하고 캡처된 상태값만 수정할수 있도록 함
    float CollideTimeRatio = CollisionProcessor->ProcessCollisions(StepTime);
    ConsumimgTimeRatio = std::min(ConsumimgTimeRatio, CollideTimeRatio);
    ReamainingTimeRatio -= ConsumimgTimeRatio;

    // 시간 업데이트
    const float SimualtedTime = StepTime * ReamainingTimeRatio;
    AccumulatedTime -= SimualtedTime;

    // 물리시스템 관리 객체 Tick
    for (auto& PhysicsObject : RegisteredObjects)
    {
        //PhysicsTick 
        auto PhysicsObjectPtr = PhysicsObject.lock();
        if (PhysicsObjectPtr && SimualtedTime > KINDA_SMALL)
        {
            PhysicsObjectPtr->TickPhysics(SimualtedTime);
        }
       
    }

    if (ReamainingTimeRatio < KINDA_SMALL)
    {
        return true;

    }
    else
    {
        LOG_FUNC_CALL("CollideSubSteps for %.3f", CollideTimeRatio);
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
            Object->SynchronizeState();
        }
    }
}




///////////////////////////////////////////////////////////

void UPhysicsSystem::PrintDebugInfo()
{
#ifdef _DEBUG
    LOG("Current Active PhysicsObejct : [%03u]", RegisteredObjects.size());
#endif
}
