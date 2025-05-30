#pragma once
#include "Math.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "DynamicAABBTree.h"

class FDynamicAABBTree;
struct FTransform;
class IPhysicsStateInternal;

#pragma region CollisionPair
struct FCollisionPair
{
    FCollisionPair(size_t InIdA, size_t InIdB)
        : TreeIdA(InIdA < InIdB ? InIdA : InIdB)
        , TreeIdB(InIdA < InIdB ? InIdB : InIdA)
        , bPrevCollided(false)
        , bConverged(false)
    {
    }
    FCollisionPair& operator=(const FCollisionPair& Other) = default;

    size_t TreeIdA;
    size_t TreeIdB;

    mutable FAccumulatedConstraint PrevConstraints;
    mutable bool bPrevCollided : 1;
    mutable bool bConverged : 1;
    //mutable bool bStepSimulateFinished : 1;
      
    bool operator==(const FCollisionPair& Other) const
    {
        return TreeIdA == Other.TreeIdA && TreeIdB == Other.TreeIdB;
    }
};

//Collisoin Pairs Hash
namespace std
{
    template<>
    struct hash<FCollisionPair>
    {
        size_t operator()(const FCollisionPair& Pair) const {
            return std::hash<size_t>()(Pair.TreeIdA) ^
                (std::hash<size_t>()(Pair.TreeIdB) << 1);
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
/// 등록된 컴포넌들의 충돌 현상을 관리
/// DynamicAABBTree를 이용해 객체의 충돌쌍을 관리하고
/// 충돌 테스트 및 충돌 반응등을 수행함
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
    void RegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent);
    void UnRegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent);

    // 정규화된 시뮬레이션 소모시간
    float SimulateCollision(const float DeltaTime);
    void UnRegisterAll();

    size_t GetRegisterComponentsCount() { return RegisteredComponents.size(); }
private:
    void Initialize();
    void Release();
    void CleanupDestroyedComponents();

    float ProcessCollisions(const float DeltaTime);

    //CCD 임계속도 비교
    bool ShouldUseCCD(const IPhysicsStateInternal * PhysicsStateInternal) const;

    //새로운 충돌쌍 업데이트
    void UpdateCollisionPairs();

    //컴포넌트 트랜스폼 업데이트
    void UpdateCollisionTransform();

    //내부 연산을 위한 데이터 구조체 생성
    void GetPhysicsParams(const std::shared_ptr<UCollisionComponentBase>& InComp, FPhysicsParameters& Result) const;

    //제약조건 기반 반복적 해결 - 충돌 반응
    void ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair,
                                            const FCollisionDetectionResult& DetectResult, const float DeltaTime);
    // 순수 좌표 기반 위치 보정 적용 
    void ApplyDirectPositionCorrection(
        const FCollisionPair& CollisionPair,
        const FCollisionDetectionResult& DetectionResult,
        float CorrectionRatio = 0.8f
    );

    /// <summary>
    ///  위치 보정 속도 편향 계산
    /// </summary>
    /// <param name="Slop"> Slop 초과의 침투만 고려,  m 단위 </param>
    /// <returns></returns>
    float CalculatePositionBiasVelocity(float PenetrationDepth, float BiasFactor, float DeltaTime, float Slop = 0.01f);


    /// <summary>
    /// AABB 겹침 정도를 통한 침투 깊이 비율 계산
    /// </summary>
    /// <param name="CollisionPair">충돌 쌍 정보</param>
    /// <returns>AABB 겹침 비율 [0.0f ~ 1.0f], 깊을수록 1.0에 가까움</returns>
    float CalculateAABBOverlapRatio(const FCollisionPair& CollisionPair) const;

    /// <summary>
    /// 두 AABB의 겹침 볼륨 계산
    /// </summary>
    /// <param name="BoundsA">첫 번째 AABB</param>
    /// <param name="BoundsB">두 번째 AABB</param>
    /// <returns>겹침 볼륨 크기</returns>
    float CalculateAABBOverlapVolume(
        const FDynamicAABBTree::AABB& BoundsA,
        const FDynamicAABBTree::AABB& BoundsB
    ) const;


    void BroadcastCollisionEvents(
        const FCollisionPair& InPair,
        const FCollisionDetectionResult& DetectResult);

    //Config Load from ini
    void LoadConfigFromIni();
public:
    void PrintTreeStructure() const;

private:
    // 하부 시스템 클래스들
    class FCollisionDetector* Detector = nullptr;
    class FCollisionResponseCalculator* ResponseCalculator = nullptr;
    class FCollisionEventDispatcher* EventDispatcher = nullptr;
    class FPositionalCorrectionCalculator* PositionCorrectionCalculator = nullptr;

    // 컴포넌트 관리
    //std::vector<FComponentData> RegisteredComponents; 
    std::unordered_map<size_t, std::weak_ptr<UCollisionComponentBase>> RegisteredComponents;
    FDynamicAABBTree* CollisionTree = nullptr;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;

private:
    float CCDVelocityThreshold = 3.0f;              // CCD 활성화 속도 임계값
    size_t InitialCollisonCapacity = 512;           // 초기 컴포넌트 및 트리 용량/
    uint16_t MaxConstraintIterations = 10;          // 제약조건 해결 최대 반복수
    float FatBoundsExtentRatio = 0.1f;             // AABB 여유 공간
};