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
#include "PhysicsDataStructures.h"
#include "CollisionShapeInternalInterface.h"
#include "PhysicsEventDispatcherInterface.h"

/// <summary>
/// 물리 시뮬레이션 시스템의 중앙 관리자
/// 
/// 핵심 책임:
/// - 물리 객체 생명주기 관리 (등록/해제)
/// - 고정 시간 단계 기반 물리 시뮬레이션
/// - 배치 연산을 통한 성능 최적화
/// - 게임-물리 간 동기화 관리
/// - 이벤트 기반 비동기 통신
/// 
/// 설계 원칙:
/// - SoA(Structure of Arrays) 기반 메모리 최적화
/// - 인터페이스 기반 의존성 주입
/// - Job 시스템을 통한 비동기 작업 처리
/// - 이벤트 큐를 통한 느슨한 결합
/// </summary>
class UPhysicsSystem : public IPhysicsStateInternal,
    public ICollisionShapeInternal,
    public IPhysicsEventDispatcher
{
#pragma region Constructor and Initialization

private:
    UPhysicsSystem() = default;
    ~UPhysicsSystem();

    // 복사 및 이동 방지
    UPhysicsSystem(const UPhysicsSystem&) = delete;
    UPhysicsSystem& operator=(const UPhysicsSystem&) = delete;
    UPhysicsSystem(UPhysicsSystem&&) = delete;
    UPhysicsSystem& operator=(UPhysicsSystem&&) = delete;

public:
    /// <summary>
    /// 싱글톤 인스턴스 접근
    /// </summary>
    static UPhysicsSystem* Get()
    {
        static UPhysicsSystem* manager = []() {
            UPhysicsSystem* instance = new UPhysicsSystem();
            instance->Initialize();
            return instance;
            }();
        return manager;
    }

    /// <summary>
    /// 물리 시스템 초기화
    /// INI 설정 로드, 메모리 할당, 하부 시스템 초기화
    /// </summary>
    void Initialize();

    /// <summary>
    /// 물리 시스템 해제 및 메모리 정리
    /// </summary>
    void Release();

    /// <summary>
    /// INI 파일에서 물리 시뮬레이션 설정값 로드
    /// </summary>
    void LoadConfigFromIni();

#pragma endregion

#pragma region Public Interface

public:
    static constexpr PhysicsID INVALID_PHYSICS_ID = 0;

    /// <summary>
    /// 물리 객체 등록
    /// </summary>
    /// <param name="Object">등록할 물리 객체</param>
    /// <returns>할당된 SoAID</returns>
    SoAID RegisterPhysicsObject(std::shared_ptr<IPhysicsObject>& Object);

    /// <summary>
    /// 물리 객체 해제
    /// </summary>
    /// <param name="id">해제할 객체 ID</param>
    void UnregisterPhysicsObject(SoAID id);

    /// <summary>
    /// 물리 시뮬레이션 메인 루프
    /// 고정 시간 단계 기반 시뮬레이션 실행
    /// </summary>
    /// <param name="DeltaTime">프레임 시간</param>
    void TickPhysics(const float DeltaTime);

    /// <summary>
    /// 디버그 정보 출력
    /// </summary>
    void PrintDebugInfo();

#pragma endregion

#pragma region Data Access Layer

public:
    /// <summary>
    /// SoAID 유효성 검사
    /// </summary>
    bool IsValidTargetID(const SoAID targetID) const;

private:
    /// <summary>
    /// SoAID를 배열 인덱스로 변환
    /// </summary>
    SoAIdx GetIdx(const SoAID targetID) const;

    /// <summary>
    /// 활성화된 PhysicsID 목록 수집
    /// CollisionProcessor와의 연동을 위한 데이터 준비
    /// </summary>
    std::vector<PhysicsID> GetActivePhysicsIDs() const;

#pragma endregion

#pragma region IPhysicsEventDispatcher Implementation

public:
    /// <summary>
    /// 충돌 이벤트 배송 요청 (IPhysicsEventDispatcher 구현)
    /// CollisionProcessor에서 생성된 이벤트를 큐에 추가
    /// </summary>
    /// <param name="Event">배송 요청할 충돌 이벤트</param>
    void AddCollisionEvent(const FPhysicsCollisionEvent& Event) override;

private:
    /// <summary>
    /// 충돌 이벤트 순환 큐 - 고정 크기로 성능 최적화
    /// 큐 크기는 InitialCollisionEventQueueSize 설정값으로 결정
    /// </summary>
    std::unique_ptr<TCircularQueue<FPhysicsCollisionEvent>> CollisionEventQueue;

    /// <summary>
    /// 배치 이벤트 처리 및 게임 로직에 이벤트 전달
    /// FinalizeSimulation()에서 호출되어 큐의 모든 이벤트를 RigidBodyComponent로 전송
    /// </summary>
    void SyncPhysicsEvents();

    /// <summary>
    /// 이벤트 큐 수동 초기화
    /// </summary>
    void ClearEventQueue();

    /// <summary>
    /// 현재 큐에 대기 중인 이벤트 수 반환 (디버깅 용도)
    /// </summary>
    size_t GetEventQueueSize() const;

    /// <summary>
    /// 개별 PhysicsObject에 단일 이벤트 전송
    /// FIFO 순서 보장을 위한 즉시 전송 방식
    /// </summary>
    void BatchSynchCollisionEvents(PhysicsID TargetPhysicsID, std::vector<FPhysicsCollisionEvent>& Event);

#pragma endregion

#pragma region Job System Management

private:
    struct FPhysicsJobRequest
    {
        FPhysicsJob* Job = nullptr;
        bool IsValid() const { return Job != nullptr; }
    };

    /// <summary>
    /// Job 메모리 관리
    /// </summary>
    std::unique_ptr<FArenaMemoryPool> JobPool;
    std::unique_ptr<TCircularQueue<FPhysicsJobRequest>> JobQueue;

public:
    /// <summary>
    /// 외부 Job 요청 인터페이스
    /// </summary>
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
    /// <summary>
    /// Job 메모리 할당 및 큐 추가
    /// </summary>
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
        newJobRequest.Job = JobPool->Allocate<JobType>(std::forward<Args>(args)...);

        if (newJobRequest.Job != nullptr)
        {
            JobQueue->Push(newJobRequest);
        }
    }

    /// <summary>
    /// Job 순차 처리
    /// </summary>
    void ProcessJobQueue();

#pragma endregion

#pragma region Physics Simulation Pipeline

private:
    /// <summary>
    /// 필요한 서브스텝 수 계산
    /// </summary>
    int CalculateRequiredSubsteps();

    /// <summary>
    /// 시뮬레이션 시작 전 준비
    /// </summary>
    void PrepareSimulation();

    /// <summary>
    /// 단일 서브스텝 시뮬레이션
    /// </summary>
    /// <param name="StepTime">서브스텝 시간</param>
    /// <returns>실제 시뮬레이션된 시간</returns>
    float SimulateSubstep(const float StepTime);

    /// <summary>
    /// 시뮬레이션 완료 후 최종 상태 적용
    /// </summary>
    void FinalizeSimulation();

    /// <summary>
    /// 물리 Tick 전파
    /// </summary>
    void BatchPhysicsTick(const float DeltaTime);

    /// <summary>
    /// 중력 배치 적용
    /// </summary>
    void BatchApplyGravity(const Vector3& gravity, float deltaTime);

    /// <summary>
    /// 속도 적분 (위치 업데이트)
    /// </summary>
    void BatchIntegrateVelocity(float deltaTime);

    /// <summary>
    /// 누적힘/토크 초기화
    /// </summary>
    void BatchResetForces();

    /// <summary>
    /// 누적힘/토크 속도변환
    /// </summary>
    void BatchApplyForces(float deltaTime);

    /// <summary>
    /// 저항 적용
    /// </summary>
    void BatchApplyDrag(float deltaTime);

    /// <summary>
    /// 개별 객체 속도 제한 적용
    /// </summary>
    void ClampLinearVelocity(float InMaxSpeed, XMVECTOR& InOutVelocity);
    void ClampAngularVelocity(float InAngularMaxSpeed, XMVECTOR& InOutAngularVelocity);

    /// <summary>
    /// 수치 안정성 검증 함수들
    /// </summary>
    bool IsValidForce(const XMVECTOR& InForce);
    bool IsValidTorque(const XMVECTOR& InTorque);
    bool IsValidLinearVelocity(const XMVECTOR& InVelocity);
    bool IsValidAngularVelocity(const XMVECTOR& InAngularVelocity);
    bool IsValidLinearAcceleration(const XMVECTOR& InAccel);
    bool IsValidAngularAcceleration(const XMVECTOR& InAngularAccel);

#pragma endregion

#pragma region Synchronization System

public:
    /// <summary>
    /// 게임 → 물리 동기화
    /// 변경된 게임 상태를 물리 시스템으로 전송
    /// </summary>
    void SyncGameToPhysics();

    /// <summary>
    /// 물리 → 게임 동기화
    /// 물리 시뮬레이션 결과를 게임 객체로 전송
    /// </summary>
    void SyncPhysicsToGame();

private:
    /// <summary>
    /// 배치 동기화 함수들
    /// </summary>
    void BatchSyncHighFrequencyData();
    void BatchSyncMidFrequencyData();
    void BatchSyncLowFrequencyData();
    void BatchSyncPhysicsResults();
    void BatchClearAllDirtyFlags();

#pragma endregion

#pragma region Subsystem Interface

public:
    /// <summary>
    /// 하부시스템 - 충돌 처리기 접근
    /// </summary>
    FCollisionProcessor* GetCollisionSubsystem()
    {
        return CollisionProcessor ? CollisionProcessor.get() : nullptr;
    }

#pragma endregion

#pragma region IPhysicsStateInternal Implementation

public:
    // === Physical Properties Access ===
    float P_GetMass(PhysicsID targetID) const override;
    float P_GetInvMass(PhysicsID targetID) const override;
    XMVECTOR P_GetRotationalInertia(PhysicsID targetID) const override;
    XMVECTOR P_GetInvRotationalInertia(PhysicsID targetID) const override;
    float P_GetRestitution(PhysicsID targetID) const override;
    float P_GetFrictionStatic(PhysicsID targetID) const override;
    float P_GetFrictionKinetic(PhysicsID targetID) const override;
    float P_GetGravityScale(PhysicsID targetID) const override;
    float P_GetMaxSpeed(PhysicsID targetID) const override;
    float P_GetMaxAngularSpeed(PhysicsID targetID) const override;

    // === Motion State Access ===
    XMVECTOR P_GetVelocity(PhysicsID targetID) const override;
    XMVECTOR P_GetAngularVelocity(PhysicsID targetID) const override;
    XMVECTOR P_GetAccumulatedForce(PhysicsID targetID) const override;
    XMVECTOR P_GetAccumulatedTorque(PhysicsID targetID) const override;

    // === Transform Access ===
    XMVECTOR P_GetWorldPosition(PhysicsID targetID) const override;
    XMVECTOR P_GetWorldRotationQuat(PhysicsID targetID) const override;
    XMVECTOR P_GetWorldScale(PhysicsID targetID) const override;
    XMMATRIX P_GetWorldTransformMatrix(PhysicsID targetID) const override;

    XMVECTOR P_GetPrevWorldPosition(PhysicsID id) const override;
    XMVECTOR P_GetPrevWorldRotationQuat(PhysicsID id) const override;
    XMVECTOR P_GetPrevWorldScale(PhysicsID id) const override;
    XMMATRIX P_GetPrevWorldTransformMatrix(PhysicsID targetID) const override;

    // === Force and Impulse Application ===
    void P_ApplyForce(PhysicsID targetID, XMVECTOR force, XMVECTOR location) override;
    void P_ApplyImpulse(PhysicsID targetID, XMVECTOR impulse, XMVECTOR location) override;

    // === Property Setters ===
    void P_SetMass(PhysicsID targetID, float mass) override;
    void P_SetInvMass(PhysicsID targetID, float invMass) override;
    void P_SetRestitution(PhysicsID targetID, float restitution) override;
    void P_SetFrictionStatic(PhysicsID targetID, float frictionStatic) override;
    void P_SetFrictionKinetic(PhysicsID targetID, float frictionKinetic) override;
    void P_SetGravityScale(PhysicsID targetID, float gravityScale) override;
    void P_SetMaxSpeed(PhysicsID targetID, float maxSpeed) override;
    void P_SetMaxAngularSpeed(PhysicsID targetID, float maxAngularSpeed) override;

    // === Transform Setters ===
    void P_SetWorldPosition(PhysicsID targetID, XMVECTOR worldPosition) override;
    void P_SetWorldRotation(PhysicsID targetID, XMVECTOR worldRotation) override;
    void P_SetWorldScale(PhysicsID targetID, XMVECTOR worldScale) override;
    void P_SetPrevWorldPosition(PhysicsID targetID, XMVECTOR worldPosition) override;
    void P_SetPrevWorldRotation(PhysicsID targetID, XMVECTOR worldRotation) override;
    void P_SetPrevWorldScale(PhysicsID targetID, XMVECTOR worldScale) override;

    // === Vector Property Setters ===
    void P_SetRotationalInertia(PhysicsID targetID, XMVECTOR rotationalInertia) override;
    void P_SetInvRotationalInertia(PhysicsID targetID, XMVECTOR invRotationalInertia) override;

    //// === Motion State Setters ===
    //void P_SetVelocity(PhysicsID targetID, XMVECTOR velocity) override;
    //void P_SetAngularVelocity(PhysicsID targetID, XMVECTOR angularVelocity) override;
    //void P_AddVelocity(PhysicsID targetID, const Vector3& deltaVelocity) override;
    //void P_AddAngularVelocity(PhysicsID targetID, const Vector3& deltaAngularVelocity) override;

    // === State Type and Control ===
    void P_SetPhysicsType(PhysicsID targetID, EPhysicsType physicsType) override;
    void P_SetPhysicsMask(PhysicsID targetID, const FPhysicsMask& physicsMask) override;
    void P_SetPhysicsActive(PhysicsID targetID, bool bActive) override;
    bool P_IsPhysicsActive(PhysicsID targetID) const override;

#pragma endregion

#pragma region ICollisionShapeInternal Implementation

public:
    // === 충돌 형상 인터페이스 ===
    ECollisionShapeType P_GetShapeType(PhysicsID id) const override;
    void P_SetShapeType(PhysicsID id, ECollisionShapeType type) override;
    XMVECTOR P_GetShapeHalfExtent(PhysicsID id) const override;
    void P_SetShapeHalfExtent(PhysicsID id, XMVECTOR extent) override;

#pragma endregion

#pragma region Configuration Members

private:
    // 물리 시뮬레이션 설정값들 (INI 파일에서 로드)
    int InitialCollisionEventQueueSize = 512;     // 충돌 이벤트 큐 크기
    int InitialPhysicsObjectCapacity = 512;       // 최초 관리 객체 메모리 크기
    int InitialPhysicsJobPoolSizeMB = 4;          // 최초 물리 작업 풀 크기
    float FixedTimeStep = 0.016f;                 // 60Hz
    float MinSubStepTickTime = 0.004f;            // 15Hz
    int MaxSubSteps = 5;                          // 최대 서브스텝 수
    int MinSubSteps = 3;                          // 최소 서브스텝 수
    std::uint8_t BatchSize = 64;                  // 배치 연산 크기

    float MaxPhysicsVelocity = 1000.0f;            // 1000m/s 음속의 약 3배 
    float MaxPhysicsAngularVelocity = 100.0f;      // 약 573도/초 (라디안/초)
    float MaxPhysicsForce = 1000000.0f;  // 1MN (메가뉴턴)
    float MaxPhysicsTorque = 100000.0f;  // 100kN·m
    float MaxPhysicsAcceleration = 10000.0f;  // 약 1000G (m/s²)
    float MaxPhysicsAngularAcceleration = 1000.0f;  // 약 57,000도/초² (라디안/초²) 

    Vector3 Gravity = -9.812f * Vector3::Up();    // 중력 벡터

    // 누적 시간 상태값
    float AccumulatedTime = 0.0f;

    // 시뮬레이션 상태
    bool bIsSimulating = false;

#pragma endregion

#pragma region Subsystem Components

private:
    // 하부 시스템들
    std::unique_ptr<FCollisionProcessor> CollisionProcessor;

    // 물리 상태 데이터 관리자
    std::unique_ptr<FPhysicsStateArrays> PhysicsStateSoA;

#pragma endregion

};