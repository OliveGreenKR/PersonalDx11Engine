#pragma once
#include "Math.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include "CollisionComponent.h"
#include "CollisionDefines.h"

class FDynamicAABBTree;
class URigidBodyComponent;
struct FTransform;


#pragma region CollisionPair
struct FCollisionPair
{
    FCollisionPair(size_t InIdA, size_t InIdB)
        : TreeIdA(InIdA < InIdB ? InIdA : InIdB)
        , TreeIdB(InIdA < InIdB ? InIdB : InIdA)
        , bPrevCollided(false)
    {
    }
    FCollisionPair& operator=(const FCollisionPair& Other) = default;

    size_t TreeIdA;
    size_t TreeIdB;

    mutable FAccumulatedConstraint PrevConstraints;
    mutable bool bPrevCollided : 1;
      
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

struct FCollisionSystemConfig
{
    bool bPhysicsSimulated = true;
    float MinimumTimeStep = 0.0016f;     // 최소 시간 간격 (약 600fps)
    float MaximumTimeStep = 0.0166f;     // 최대 시간 간격 (약 60fps)
    float CCDVelocityThreshold = 3.0f;     // CCD 활성화 속도 임계값
    //fixedTimeStep
    bool bUseFixedTimestep = true;       // 고정 타임스텝 사용 여부
    float FixedTimeStep = 0.016f;        // 고정 타임스텝 크기 (60Hz)
    int MaxSubSteps = 5;                 // 최대 서브스텝 수

    // AABB Tree 관련 설정
    size_t InitialCapacity = 512;       // 초기 컴포넌트 및 트리 용량
    float AABBMargin = 0.1f;             // AABB 여유 공간
};

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
    [[deprecated("Use Another signature of RegisterCollision")]]
    void RegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent,
                              const std::shared_ptr<URigidBodyComponent>& InRigidBody);

    void RegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent);
    void UnRegisterCollision(std::shared_ptr<UCollisionComponentBase>& NewComponent);

    void Tick(const float DeltaTime);
    void UnRegisterAll();

    size_t GetRegisterComponentsCount() { return RegisteredComponents.size(); }

    // 충돌 처리 후 정규화된 충돌시간 
    float ProcessCollisions(const float DeltaTime);

public:
    FCollisionSystemConfig Config;

private:
    void Initialize();
    void Release();
    void CleanupDestroyedComponents();

    void SimulateStep(float stepDeltaTime);



    //CCD 임계속도 비교
    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    //새로운 충돌쌍 업데이트
    void UpdateCollisionPairs();

    //컴포넌트 트랜스폼 업데이트
    void UpdateCollisionTransform();

    void GetPhysicsParams(const std::shared_ptr<UCollisionComponentBase>& InComp, FPhysicsParameters& Result) const;

    void ApplyPositionCorrection(
        const std::shared_ptr<UCollisionComponentBase>& ComponentA,
        const std::shared_ptr<UCollisionComponentBase>& ComponentB,
        const FCollisionDetectionResult& DetectResult,
        const float DeltaTime);

    //제약조건 기반 반복적 해결
    void ApplyCollisionResponseByContraints(const FCollisionPair& CollisionPair,
                                            const FCollisionDetectionResult& DetectResult);

    void BroadcastCollisionEvents(
        const FCollisionPair& InPair,
        const FCollisionDetectionResult& DetectResult);

    //Config Load from ini
    void LoadConfigFromIni();
public:
    void PrintTreeStructure();

private:
    // 하부 시스템 클래스들
    class FCollisionDetector* Detector = nullptr;
    class FCollisionResponseCalculator* ResponseCalculator = nullptr;
    class FCollisionEventDispatcher* EventDispatcher = nullptr;

    // 고정 타임스텝 관련 변수
    float AccumulatedTime = 0.0f;

    // 컴포넌트 관리
    //std::vector<FComponentData> RegisteredComponents; 
    std::unordered_map<size_t, std::weak_ptr<UCollisionComponentBase>> RegisteredComponents;
    FDynamicAABBTree* CollisionTree = nullptr;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;
};