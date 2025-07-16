#pragma once
#include "Math.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include "CollisionDefines.h"
#include "DynamicAABBTree.h"

class FDynamicAABBTree;
struct FTransform;
class IPhysicsStateInternal;
class ICollisionShapeInternal;

using PhysicsID = std::uint32_t;

#pragma region CollisionPair
struct FCollisionPair
{
    FCollisionPair(PhysicsID InIdA, PhysicsID InIdB)
        : PhysicsIdA(InIdA < InIdB ? InIdA : InIdB)
        , PhysicsIdB(InIdA < InIdB ? InIdB : InIdA)
        , bPrevCollided(false)
        , bConverged(false)
    {
    }
    FCollisionPair& operator=(const FCollisionPair& Other) = default;

    PhysicsID PhysicsIdA;
    PhysicsID PhysicsIdB;

    mutable FAccumulatedConstraint PrevConstraints;
    mutable bool bPrevCollided : 1;
    mutable bool bConverged : 1;

    bool operator==(const FCollisionPair& Other) const
    {
        return PhysicsIdA == Other.PhysicsIdA && PhysicsIdB == Other.PhysicsIdB;
    }
};

//Collisoin Pairs Hash
namespace std
{
    template<>
    struct hash<FCollisionPair>
    {
        size_t operator()(const FCollisionPair& Pair) const {
            return std::hash<PhysicsID>()(Pair.PhysicsIdA) ^
                (std::hash<PhysicsID>()(Pair.PhysicsIdB) << 1);
        }
    };
}
#pragma endregion

struct FContactPoint
{
    Vector3 Position;        // 접촉점 위치
    Vector3 Normal;         // 접촉면 노말
    float Penetration;      // 침투 깊이
    float AccumulatedNormalImpulse;   // 누적된 수직 충격량
    float AccumulatedTangentImpulse;  // 누적된 접선 충격량
};

/// <summary>
/// PhysicsSystem 기반 충돌 처리 시스템
/// PhysicsID를 통해 형상 정보를 조회하고 충돌 검출/해결을 수행
/// </summary>
class FCollisionProcessor
{
private:
    friend class UPhysicsSystem;

    // 복사 및 이동 방지
    FCollisionProcessor(const FCollisionProcessor&) = delete;
    FCollisionProcessor& operator=(const FCollisionProcessor&) = delete;
    FCollisionProcessor(FCollisionProcessor&&) = delete;
    FCollisionProcessor& operator=(FCollisionProcessor&&) = delete;

    // 생성자/소멸자
    FCollisionProcessor() = default;
    ~FCollisionProcessor();

public:
    // PhysicsSystem 기반 충돌 처리
    float ProcessCollisions(const std::vector<PhysicsID>& activePhysicsIDs, float DeltaTime);
    void UpdateSpatialPartitioning(const std::vector<PhysicsID>& activePhysicsIDs);

    // 시스템 관리
    void UnRegisterAll();
    void LoadConfigFromIni();

#pragma region Physics System Integration
private:
    // PhysicsSystem 인터페이스 참조
    IPhysicsStateInternal* PhysicsStateInterface = nullptr;
    ICollisionShapeInternal* ShapeInterface = nullptr;

    // PhysicsSystem에서 형상 데이터 조회
    void GetCollisionShapeData(PhysicsID id, FCollisionShapeData& outData) const;

    // PhysicsSystem에서 물리 매개변수 조회
    void GetPhysicsParams(PhysicsID id, FPhysicsParameters& outParams) const;

    // PhysicsID 기반 직접 충돌 검출 (Proxy 없이)
    FCollisionDetectionResult DetectCollisionBetweenPhysicsObjects(
        PhysicsID idA, PhysicsID idB, float deltaTime) const;

    // CCD 충돌 검출
    FCollisionDetectionResult DetectCollisionCCD_PhysicsObjects(
        PhysicsID idA, PhysicsID idB, float deltaTime) const;
#pragma endregion

#pragma region Core Collision Processing
private:
    void Initialize();
    void Release();

    // CCD 임계속도 비교
    bool ShouldUseCCD(PhysicsID id) const;

    // 새로운 충돌쌍 업데이트
    void UpdateCollisionPairs(const std::vector<PhysicsID>& activePhysicsIDs);

    // 제약조건 기반 반복적 해결 - 충돌 반응
    void ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair,
                                            const FCollisionDetectionResult& DetectResult, const float DeltaTime);

    // 순수 좌표 기반 위치 보정 적용 
    void ApplyDirectPositionCorrection(
        const FCollisionPair& CollisionPair,
        const FCollisionDetectionResult& DetectionResult,
        float CorrectionRatio = 0.8f
    );

    // 위치 보정 속도 편향 계산
    float CalculatePositionBiasVelocity(float PenetrationDepth, float BiasFactor, float DeltaTime, float Slop = 0.01f);

    // AABB 겹침 정도를 통한 침투 깊이 비율 계산
    float CalculateAABBOverlapRatio(const FCollisionPair& CollisionPair) const;
    float CalculateAABBOverlapVolume(const FDynamicAABBTree::AABB& BoundsA, const FDynamicAABBTree::AABB& BoundsB) const;

    // 충돌 이벤트 전파 (향후 구현)
    void BroadcastCollisionEvents(const FCollisionPair& InPair, const FCollisionDetectionResult& DetectionResult);
#pragma endregion

#pragma region Debug
public:
    void PrintTreeStructure() const;
#pragma endregion

#pragma region Member Variables
private:
    // 공간 분할 트리 (PhysicsID와 AABB 저장)
    std::unique_ptr<FDynamicAABBTree> CollisionTree;

    // 활성 충돌 쌍 관리
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;

    // 충돌 검출 및 해결 시스템
    std::unique_ptr<class FCollisionDetector> Detector;
    std::unique_ptr<class FCollisionResponseCalculator> ResponseCalculator;
    std::unique_ptr<class FCollisionEventCalculator> EventCalculator;
    std::unique_ptr<class FCollisionPositionalCorrectionCalculator> PositionCorrectionCalculator;

    // 설정값
    float CCDVelocityThreshold = 3.0f;
    int InitialCollisonCapacity = 512;
    int MaxConstraintIterations = 10;
    float FatBoundsExtentRatio = 0.1f;
#pragma endregion
};