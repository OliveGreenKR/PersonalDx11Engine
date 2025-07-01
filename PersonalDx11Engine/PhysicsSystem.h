#pragma once
#include <vector>
#include "PhysicsObjectInterface.h"
#include <memory>
#include "CollisionProcessor.h"
#include "DynamicCircularQueue.h"
#include <type_traits>
#include "Debug.h"
#include "PhysicsStateSoA.h"
#include "ArenaMemoryPool.h"
#include "PhysicsJob.h"

class UPhysicsSystem
{
    
private:
    UPhysicsSystem();
    ~UPhysicsSystem();

    // 복사 및 이동 방지
    UPhysicsSystem(const UPhysicsSystem&) = delete;
    UPhysicsSystem& operator=(const UPhysicsSystem&) = delete;
    UPhysicsSystem(UPhysicsSystem&&) = delete;
    UPhysicsSystem& operator=(UPhysicsSystem&&) = delete;

private:
    struct FPhysicsJobRequest
    {
        FPhysicsJob* PhysicsJob;
    };

private:
    // 등록된 물리 객체들
    std::vector<std::weak_ptr<IPhysicsObject>> RegisteredObjects;

    //등록된 물리 상태값
    std::unique_ptr<FPhysicsStateArrays> PhysicsStateSoA;

    //물리 작업 풀
    FArenaMemoryPool PhysicsJobPool;

    //물리 잡업 큐
    TCircularQueue<FPhysicsJobRequest> JobQueue;

public:
    template<typename T, typename ...Args,
        typename = std::enable_if_t<
        std::conjunction_v<
            std::is_base_of<FPhysicsJob, T>,
            std::is_constructible<T, Args...>>
            >
        >
        FPhysicsJobRequest AcquireJob(const std::shared_ptr<class IPhysicsObject>& Target, Args ...args)
    {
        FPhysicsJobRequest newJobRequest;
        if (!Target)
        {
            return newJobRequest;
        }

        newJobRequest.PhysicsJob = PhysicsJobPool.Allocate<T>(Target->GetPhysicsID(),
                                                              std::forward<Args>(args)...);
        if (!newJobRequest.PhysicsJob)
        {
            LOG_ERROR("Too Many Physics Job Requested! Pool is Full");
            return newJobRequest;
        }

        return newJobRequest;
    }

    void RequestPhysicsJob(const FPhysicsJobRequest& RequestedJob);

public:
    static UPhysicsSystem* Get()
    {
        static UPhysicsSystem* manager = [](){
            UPhysicsSystem* instance = new UPhysicsSystem();
            instance->Initialzie();
            return instance;
        }();

        return manager;
    }
    //하부시스템 - 충돌
    static FCollisionProcessor* GetCollisionSubsystem()
    {
        static FCollisionProcessor* instance = []() {
            FCollisionProcessor* collision = new FCollisionProcessor();
            collision->Initialize();
            return collision;
            }();
        return instance;
    }

    // 물리 객체 등록/해제
    void RegisterPhysicsObject(std::shared_ptr<IPhysicsObject>& Object);
    void UnregisterPhysicsObject(std::shared_ptr<IPhysicsObject>& Object);

    // 메인 물리 업데이트 (게임 루프에서 호출)
    void TickPhysics(const float DeltaTime);
#pragma region Debug
    void PrintDebugInfo();
#pragma endregion

private:
    void Initialzie();

    void Release();

    void LoadConfigFromIni();

    // 필요한 서브스텝 수 계산
    int CalculateRequiredSubsteps();

    // 시뮬레이션 시작 전 준비
    void PrepareSimulation();

    // 단일 서브스텝 시뮬레이션
    bool SimulateSubstep(const float TimeStep);

    // 시뮬레이션 완료 후 최종 상태 적용
    void FinalizeSimulation();

    //수치안정성 함수
    bool IsValidForce(const Vector3& InForce);
    bool IsValidTorque(const Vector3& InTorque);
    bool IsValidVelocity(const Vector3& InVelocity);
    bool IsValidAngularVelocity(const Vector3& InAngularVelocity);
    bool IsValidAcceleration(const Vector3& InAccel);
    bool IsValidAngularAcceleration(const Vector3& InAngularAccel);

    void ClampVelocities(SoAID ObjectID);
    void ClampLinearVelocity(float InMaxSpeed, XMVECTOR& InOutVelocity);
    void ClampAngularVelocity(float InAngularMaxSpeed, XMVECTOR& InOutAngularVelocity);

private:
    // 물리 시뮬레이션 설정
    int InitialPhysicsObjectCapacity = 512; //최초 관리 객체 메모리 크기
    int InitialPhysicsJobPoolSizeMB = 4; // 최초 물리 작업 풀 크기
    float FixedTimeStep = 0.016f;  // 60Hz
    float MinSubStepTickTime = 0.004f; // 15Hz
    int MaxSubSteps = 5;                 // 최대 서브스텝 수
    int MinSubSteps = 3;                 // 최소 서브스텝 수 - 연속적인 충돌을 처리하기 위함

    //누적 tickTime 상태값
    float AccumulatedTime = 0.0f;

    // 시뮬레이션 상태
    bool bIsSimulating = false;
};