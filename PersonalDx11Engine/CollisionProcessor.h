#pragma once

#include "Math.h"
#include "CollisionDefines.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

// Forward Declarations
class IPhysicsStateInternal;
class ICollisionShapeInternal;
class FCollisionDetector;
class FCollisionResponseCalculator;
class FCollisionEventCalculator;
class FCollisionPositionCorrectionCalculator;
class FDynamicAABBTree;

using PhysicsID = std::uint32_t;

#pragma region Data Structures

// PhysicsID 기반 충돌 쌍 (Component 참조 완전 제거)
struct FCollisionPair
{
    PhysicsID PhysicsIdA = 0;
    PhysicsID PhysicsIdB = 0;

    // 충돌 상태 추적
    mutable bool bPrevCollided = false;
    mutable bool bConverged = false;

    // Warm Starting용 제약 누적값
    mutable FAccumulatedConstraint PrevConstraints;

    FCollisionPair() = default;
    FCollisionPair(PhysicsID IdA, PhysicsID IdB)
        : PhysicsIdA(IdA < IdB ? IdA : IdB)
        , PhysicsIdB(IdA < IdB ? IdB : IdA)
    {
    }

    bool operator==(const FCollisionPair& Other) const
    {
        return PhysicsIdA == Other.PhysicsIdA && PhysicsIdB == Other.PhysicsIdB;
    }

    bool IsValid() const { return PhysicsIdA != 0 && PhysicsIdB != 0 && PhysicsIdA != PhysicsIdB; }
};

// FCollisionPair 해시 함수
struct FCollisionPairHash
{
    std::size_t operator()(const FCollisionPair& Pair) const
    {
        return std::hash<std::uint64_t>{}(
            (static_cast<std::uint64_t>(Pair.PhysicsIdA) << 32) | Pair.PhysicsIdB
            );
    }
};

#pragma endregion

#pragma region FCollisionProcessor Class

/**
 * PhysicsSystem 전용 충돌 처리 엔진
 *
 * 핵심 설계 원칙:
 * 1. PhysicsID 기반 순수 처리 - Component 의존성 제거
 * 2. 인터페이스 기반 데이터 접근 - 의존성 주입 패턴
 * 3. 상태 없는 프레임 처리 - 외부 활성 ID 목록 기반
 * 4. SimulateSubStep 전용 설계 - 정확한 시간 계산
 */
class FCollisionProcessor
{

#pragma region Public Interface (PhysicsSystem 전용)

public:
    /// <summary>
    /// 핵심 충돌 처리 인터페이스 (PhysicsSystem::SimulateSubStep()에서 호출되는 메인 함수)
    /// </summary>
    /// <param name="ActivePhysicsIDs">이번 프레임에 활성화된 PhysicsID 목록</param>
    /// <param name="DeltaTime">서브스텝 시간</param>
    /// <returns>정규화된 시뮬레이션 시간 (CCD 적용 시 1.0보다 작음, 일반적으로 0.0~1.0)</returns>
    float ProcessCollisions(const std::vector<PhysicsID>& ActivePhysicsIDs, float DeltaTime);

    /// <summary>
    /// 공간 분할 트리 업데이트 (활성 PhysicsID 목록을 기반으로 AABB 트리를 갱신)
    /// </summary>
    /// <param name="ActivePhysicsIDs">활성화된 PhysicsID 목록</param>
    void UpdateSpatialPartitioning(const std::vector<PhysicsID>& ActivePhysicsIDs);

    /// <summary>
    /// INI 파일에서 충돌 처리 설정값 로드
    /// </summary>
    void LoadConfigFromIni();

    /// <summary>
    /// 모든 충돌 쌍 및 공간 분할 트리 초기화
    /// </summary>
    /// <summary>
    /// 모든 충돌 쌍 및 공간 분할 트리 초기화
    /// </summary>
    void UnRegisterAll();

    /// <summary>
    /// 충돌 처리 시스템 초기화 상태 확인
    /// </summary>
    /// <returns>시스템이 올바르게 초기화되었는지 여부</returns>
    bool IsInitialized() const;

#pragma endregion

#pragma region Constructor and Initialization

public:
    FCollisionProcessor() = default;
    ~FCollisionProcessor();

    // 복사/이동 금지
    FCollisionProcessor(const FCollisionProcessor&) = delete;
    FCollisionProcessor& operator=(const FCollisionProcessor&) = delete;
    FCollisionProcessor(FCollisionProcessor&&) = delete;
    FCollisionProcessor& operator=(FCollisionProcessor&&) = delete;

    /// <summary>
    /// 의존성 주입을 통한 CollisionProcessor 초기화
    /// </summary>
    /// <param name="PhysicsStateInterface">물리 상태 인터페이스</param>
    /// <param name="ShapeInterface">충돌 형상 인터페이스</param>
    /// <returns>초기화 성공 여부</returns>
    bool Initialize(IPhysicsStateInternal* PhysicsStateInterface, ICollisionShapeInternal* ShapeInterface);

    /// <summary>
    /// 시스템 해제 및 메모리 정리
    /// </summary>
    void Release();

#pragma endregion

#pragma region Dependency Injection

private:
    // 인터페이스 기반 의존성 주입 (Initialize에서 설정)
    IPhysicsStateInternal* PhysicsStateInterface = nullptr;
    ICollisionShapeInternal* ShapeInterface = nullptr;

    // 초기화 상태 추적
    bool bIsInitialized = false;

#pragma endregion

#pragma region Data Access Layer

private:
    // 인터페이스를 통한 데이터 조회 (PhysicsID → SIMD 구조체 변환)
    void GetCollisionShapeData(PhysicsID Id, FCollisionShapeData& OutData) const;
    void GetPhysicsParams(PhysicsID Id, FPhysicsParameters& OutParams) const;

    // 월드 변환 조회 (SIMD 형식)
    XMVECTOR GetCurrentWorldPosition(PhysicsID Id) const;
    XMVECTOR GetCurrentWorldRotation(PhysicsID Id) const;  // Quaternion
    XMVECTOR GetCurrentWorldScale(PhysicsID Id) const;
    XMVECTOR GetPrevWorldPosition(PhysicsID Id) const;
    XMVECTOR GetPrevWorldRotation(PhysicsID Id) const;
    XMVECTOR GetPrevWorldScale(PhysicsID Id) const;

#pragma endregion

#pragma region Collision Processing Pipeline

private:
    // 충돌 쌍 관리 및 브로드페이즈
    void UpdateCollisionPairs(const std::vector<PhysicsID>& ActivePhysicsIDs);

    // 정밀 충돌 감지 단계, 최소 충돌 감지 시간 반환
    float PerformNarrowphaseDetection(const std::vector<PhysicsID>& ActivePhysicsIDs,
                                      float DeltaTime,
                                      std::vector<FCollisionPair>& OutCollidingPairs,
                                      std::vector<FCollisionDetectionResult>& OutDetectionResults);

    // 시간 기반 충돌 필터링 (CCD 최적화)
    void FilterCollisionsByTime(std::vector<FCollisionPair>& CollidingPairs,
                                std::vector<FCollisionDetectionResult>& DetectionResults,
                                float TargetTime);

    // 충돌 반응 및 해결 단계  
    void ProcessCollisionResponse(const std::vector<FCollisionPair>& CollidingPairs,
                                   const std::vector<FCollisionDetectionResult>& DetectionResults,
                                   float DeltaTime);

    // 이벤트 생성 단계
    void GenerateCollisionEvents(const std::vector<FCollisionPair>& CollidingPairs,
                                 const std::vector<FCollisionDetectionResult>& DetectionResults);

    // 충돌 반응 처리 헬퍼들
    void ApplyDirectPositionCorrections(const std::vector<FCollisionPair>& CollidingPairs,
                                        const std::vector<FCollisionDetectionResult>& DetectionResults);
    void ApplyIterativeConstraintSolver(const std::vector<FCollisionPair>& CollidingPairs,
                                        const std::vector<FCollisionDetectionResult>& DetectionResults,
                                        float EffectiveDeltaTime);
    void UpdateCollisionStates(const std::vector<FCollisionPair>& CollidingPairs,
                               const std::vector<FCollisionDetectionResult>& DetectionResults);

    // CCD 판단 및 개별 쌍 처리
    bool ShouldUseCCD(PhysicsID Id) const;
    void ApplyCollisionResponseByConstraints(
        const FCollisionPair& Pair,
        const FCollisionDetectionResult& Result,
        float DeltaTime
    );
    void ApplyDirectPositionCorrection(
        const FCollisionPair& Pair,
        const FCollisionDetectionResult& Result,
        float CorrectionRatio
    );

    // 유틸리티
    float CalculateAABBOverlapRatio(const FCollisionPair& Pair) const;

#pragma endregion

#pragma region Event Generation

private:
    // 개별 충돌 이벤트 생성 헬퍼
    void GenerateCollisionEvent(const FCollisionPair& Pair, const FCollisionDetectionResult& Result);
    void GenerateCollisionExitEvent(const FCollisionPair& Pair);

#pragma endregion

#pragma region Subsystem Components

private:
    // 완성된 하부 시스템들 (SIMD 최적화 완료)
    std::unique_ptr<FCollisionDetector> Detector;
    std::unique_ptr<FCollisionResponseCalculator> ResponseCalculator;
    std::unique_ptr<FCollisionEventCalculator> EventCalculator;
    std::unique_ptr<FCollisionPositionCorrectionCalculator> PositionCorrectionCalculator;

    // 공간 분할 시스템 (PhysicsID 기반)
    std::unique_ptr<FDynamicAABBTree> CollisionTree;

#pragma endregion

#pragma region Collision Pair Management

private:
    // 활성 충돌 쌍 관리 (PhysicsID 기반, bPrevCollided로 이전 상태 추적)
    std::unordered_set<FCollisionPair, FCollisionPairHash> ActiveCollisionPairs;

    // PhysicsID ↔ NodeID 매핑 (DynamicAABBTree와의 연결)
    std::unordered_map<PhysicsID, size_t> PhysicsIdToNodeId;
    std::unordered_map<size_t, PhysicsID> NodeIdToPhysicsId;

#pragma endregion

#pragma region Configuration

private:
    // 설정값들 (INI 파일에서 로드)
    float CCDVelocityThreshold = 10.0f;
    std::uint32_t InitialCollisionCapacity = 512;
    std::uint32_t MaxConstraintIterations = 5;
    float FatBoundsExtentRatio = 0.1f;
    float PositionCorrectionBias = 0.2f;
    float WarmStartingDamping = 0.8f;
    float MinConstraintLambda = 1.0f;

#pragma endregion

#pragma region Utility Functions

private:
    bool IsValidPhysicsID(PhysicsID Id) const { return Id != 0; }
    bool IsInterfaceValid() const { return PhysicsStateInterface != nullptr && ShapeInterface != nullptr; }

    /// <summary>
    /// 위치 편향 속도 계산 (침투 보정용 Baumgarte 안정화)
    /// </summary>
    /// <param name="PenetrationDepth">침투 깊이</param>
    /// <param name="BiasFactor">편향 계수</param>
    /// <param name="DeltaTime">시뮬레이션 시간</param>
    /// <param name="Slop">허용 침투량</param>
    /// <returns>편향 속도</returns>
    float CalculatePositionBiasVelocity(
        float PenetrationDepth,
        float BiasFactor,
        float DeltaTime,
        float Slop
    ) const;
    //형상에 따른 AABB 생성
    FMAABB CalculateAABBFromShape(XMVECTOR Pos, XMVECTOR Rot, XMVECTOR HalfExtent, ECollisionShapeType ShapeType) const;

#pragma endregion

};

#pragma endregion