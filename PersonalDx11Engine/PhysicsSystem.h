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
#include "PhysicsStateInternalInterface.h"

class UPhysicsSystem : public IPhysicsStateInternal
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
        FPhysicsJob* Job = nullptr;

        bool IsValid() const { return Job != nullptr; }
    };

public:
    static constexpr PhysicsID INVALID_PHYSICS_ID = 0;
private:
    // 물리 상태 데이터 관리자
    FPhysicsStateArrays PhysicsStateSoA;

    // Job 메모리 관리
    FArenaMemoryPool JobPool;

    //물리 잡업 큐
    TCircularQueue<FPhysicsJobRequest> JobQueue;

public:
    //외부 직업 요청 인터페이스
    template<typename JobType, typename... Args,
        typename = std::enable_if_t<
        std::conjunction_v<
        std::is_base_of<FPhysicsJob, JobType>,
        std::is_constructible<JobType, Args...>
        >
        >
    >
    void RequestPhysicsJob(Args&&... args)
    {
        AcquireJob<JobType>(std::forward<Args>(args)...);
    }

private:

    template<typename JobType, typename... Args,
        typename = std::enable_if_t<
        std::conjunction_v<
        std::is_base_of<FPhysicsJob, JobType>,
        std::is_constructible<JobType, Args...>
        >
        >
    >
    void AcquireJob(Args&&... args)
    {
        FPhysicsJobRequest newJobRequest;
        newJobRequest.Job = JobPool.Allocate<JobType>(std::forward<Args>(args)...);

        if (newJobRequest.Job != nullptr)
        {
            JobQueue.push(newJobRequest);
        }
    }

#pragma region IPhysicsStateInteranl
public :
    // 물리 속성 접근자 (PhysicsID 기반)
    float P_GetMass(PhysicsID targetID) const override;
    float P_GetInvMass(PhysicsID targetID) const override;
    Vector3 P_GetRotationalInertia(PhysicsID targetID) const override;
    Vector3 P_GetInvRotationalInertia(PhysicsID targetID) const override;
    float P_GetRestitution(PhysicsID targetID) const override;
    float P_GetFrictionStatic(PhysicsID targetID) const override;
    float P_GetFrictionKinetic(PhysicsID targetID) const override;
    float P_GetGravityScale(PhysicsID targetID) const override;
    float P_GetMaxSpeed(PhysicsID targetID) const override;
    float P_GetMaxAngularSpeed(PhysicsID targetID) const override;

    // 운동 상태 접근자 (PhysicsID 기반)
    Vector3 P_GetVelocity(PhysicsID targetID) const override;
    Vector3 P_GetAngularVelocity(PhysicsID targetID) const override;
    Vector3 P_GetAccumulatedForce(PhysicsID targetID) const override;
    Vector3 P_GetAccumulatedTorque(PhysicsID targetID) const override;

    // 트랜스폼 접근자 (PhysicsID 기반)
    FTransform P_GetWorldTransform(PhysicsID targetID) const override;
    Vector3 P_GetWorldPosition(PhysicsID targetID) const override;
    Quaternion P_GetWorldRotation(PhysicsID targetID) const override;
    Vector3 P_GetWorldScale(PhysicsID targetID) const override;

    // 상태 타입 및 마스크 접근자
    EPhysicsType P_GetPhysicsType(PhysicsID targetID) const override;
    FPhysicsMask P_GetPhysicsMask(PhysicsID targetID) const override;

    // 운동 상태 설정자 (PhysicsID 기반)
    void P_SetVelocity(PhysicsID targetID, const Vector3& velocity) override;
    void P_AddVelocity(PhysicsID targetID, const Vector3& deltaVelocity) override;
    void P_SetAngularVelocity(PhysicsID targetID, const Vector3& angularVelocity) override;
    void P_AddAngularVelocity(PhysicsID targetID, const Vector3& deltaAngularVelocity) override;

    // 트랜스폼 설정자 (PhysicsID 기반)
    void P_SetWorldPosition(PhysicsID targetID, const Vector3& position) override;
    void P_SetWorldRotation(PhysicsID targetID, const Quaternion& rotation) override;
    void P_SetWorldScale(PhysicsID targetID, const Vector3& scale) override;

    // 힘/충격 적용 (PhysicsID 기반)
    void P_ApplyForce(PhysicsID targetID, const Vector3& force, const Vector3& location) override;
    void P_ApplyImpulse(PhysicsID targetID, const Vector3& impulse, const Vector3& location) override;

    // 물리 속성 설정자 (PhysicsID 기반)
    void P_SetMass(PhysicsID targetID, float mass) override;
    void P_SetInvMass(PhysicsID targetID, float invMass) override;
    void P_SetRotationalInertia(PhysicsID targetID, const Vector3& rotationalInertia) override;
    void P_SetInvRotationalInertia(PhysicsID targetID, const Vector3& invRotationalInertia) override;
    void P_SetRestitution(PhysicsID targetID, float restitution) override;
    void P_SetFrictionStatic(PhysicsID targetID, float frictionStatic) override;
    void P_SetFrictionKinetic(PhysicsID targetID, float frictionKinetic) override;
    void P_SetGravityScale(PhysicsID targetID, float gravityScale) override;
    void P_SetMaxSpeed(PhysicsID targetID, float maxSpeed) override;
    void P_SetMaxAngularSpeed(PhysicsID targetID, float maxAngularSpeed) override;

    // 상태 타입 및 마스크 설정자
    void P_SetPhysicsType(PhysicsID targetID, EPhysicsType physicsType) override;
    void P_SetPhysicsMask(PhysicsID targetID, const FPhysicsMask& physicsMask) override;

    // 활성화 제어
    void P_SetPhysicsActive(PhysicsID targetID, bool bActive) override;
    bool P_IsPhysicsActive(PhysicsID targetID) const override;
    void P_SetCollisionActive(PhysicsID targetID, bool bActive) override;
    bool P_IsCollisionActive(PhysicsID targetID) const override;
#pragma endregion

public:
    static UPhysicsSystem* Get()
    {
        static UPhysicsSystem* manager = [](){
            UPhysicsSystem* instance = new UPhysicsSystem();
            instance->Initialize();
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
    PhysicsID RegisterPhysicsObject(std::shared_ptr<IPhysicsObject>& Object);
    void UnregisterPhysicsObject(PhysicsID id);

    // 메인 물리 업데이트 (게임 루프에서 호출)
    void TickPhysics(const float DeltaTime);

#pragma region Debug
    void PrintDebugInfo();
#pragma endregion

private:
    void Initialize();
    void Release();

    //Configure File Load
    void LoadConfigFromIni();

    // 필요한 서브스텝 수 계산
    int CalculateRequiredSubsteps();

    // 시뮬레이션 시작 전 준비
    void PrepareSimulation();

    /// <summary>
    /// 단일 서브스텝 시뮬레이션
    /// </summary>
    /// <returns> 사용한 SimulatedTime 반환 </returns>
    float SimulateSubstep(const float TimeStep);

    // 시뮬레이션 완료 후 최종 상태 적용
    void FinalizeSimulation();

    //Job 순차 처리
    void ProcessJobQueue();

private:

    SoAIdx GetIdx(const SoAID targetID) const;
    bool IsValidTargetID(const SoAID targetID) const;

#pragma region Batch to All PhyscisObj
private:
    //물리 틱 전파
    void BatchPhysicsTick(const float DeltaTime);

    //중력 적용
    void BatchApplyGravity(const Vector3& gravity, float deltaTime);

    //내부에서 속도제한 설정 속도를 + 속도 통한 위치 적분 
    void BatchIntegrateVelocity(float deltaTime);

    //누적힘/토크 초기화
    void BatchResetForces();

    //저항 적용
    void BatchApplyDrag(float deltaTime);

    //물리 상태 동기화
    void BatchSynchronizeState();
#pragma endregion
private:
    //수치안정성 함수
    bool IsValidForce(const XMVECTOR& InForce);
    bool IsValidTorque(const XMVECTOR& InTorque);
    bool IsValidLinearVelocity(const XMVECTOR& InVelocity);
    bool IsValidAngularVelocity(const XMVECTOR& InAngularVelocity);
    bool IsValidLinearAcceleration(const XMVECTOR& InAccel);
    bool IsValidAngularAcceleration(const XMVECTOR& InAngularAccel);


    /// <summary>
    /// 개별 객체 속도 제한 적용
    /// </summary>
    void ClampLinearVelocity(float InMaxSpeed, XMVECTOR& InOutVelocity);
    void ClampAngularVelocity(float InAngularMaxSpeed, XMVECTOR& InOutAngularVelocity);


private:
    // 물리 시뮬레이션 설정
    int InitialPhysicsObjectCapacity = 512; //최초 관리 객체 메모리 크기
    int InitialPhysicsJobPoolSize = 4; // 최초 물리 작업 풀 크기
    float FixedTimeStep = 0.016f;  // 60Hz
    float MinSubStepTickTime = 0.004f; // 15Hz
    int MaxSubSteps = 5;                 // 최대 서브스텝 수
    int MinSubSteps = 3;                 // 최소 서브스텝 수 - 연속적인 충돌을 처리하기 위함
    std::uint8_t BatchSize = 64;

    Vector3 Gravity = -9.812f * Vector3::Up();

    //누적 tickTime 상태값
    float AccumulatedTime = 0.0f;

    // 시뮬레이션 상태
    bool bIsSimulating = false;

};