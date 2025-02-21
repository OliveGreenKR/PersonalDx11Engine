#pragma once
#include "Math.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include "CollisionComponent.h"


class FDynamicAABBTree;
class URigidBodyComponent;
struct FTransform;

#pragma region CollisionPair
struct FCollisionPair
{
    FCollisionPair(size_t InIndexA, size_t InIndexB)
        : IndexA(InIndexA < InIndexB ? InIndexA : InIndexB)
        , IndexB(InIndexA < InIndexB ? InIndexB : InIndexA)
        , bPrevCollided(false)
    {}

    FCollisionPair(const FCollisionPair& Other) = default;
    FCollisionPair& operator=(const FCollisionPair& Other) = default;

    size_t IndexA;
    size_t IndexB;
    float ContactTime = 0.0f;   //추돌 지속 시간
    Vector3 LastNormal;         //이전 충돌 노말 
    float LastPenetration;      //이전 침투 깊이 

    bool bPrevCollided : 1;     //이전 충돌 여부

    bool operator==(const FCollisionPair& Other) const
    {
        return IndexA == Other.IndexA && IndexB == Other.IndexB;
    }
};

namespace std
{
    template<>
    struct hash<FCollisionPair>
    {
        size_t operator()(const FCollisionPair& Pair) const {
            return ((Pair.IndexA + Pair.IndexB) * (Pair.IndexA + Pair.IndexB + 1) / 2) + Pair.IndexB;
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
    
    int ConstraintInterations = 15;
    // AABB Tree 관련 설정
    size_t InitialCapacity = 1024;       // 초기 컴포넌트 및 트리 용량
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
class UCollisionManager
{
private:
    // 복사 및 이동 방지
    UCollisionManager(const UCollisionManager&) = delete;
    UCollisionManager& operator=(const UCollisionManager&) = delete;
    UCollisionManager(UCollisionManager&&) = delete;
    UCollisionManager& operator=(UCollisionManager&&) = delete;

    // 생성자/소멸자
    UCollisionManager() = default;
    ~UCollisionManager();

private:
    //컴포넌트 관리 구조체
    struct FComponentData
    {
        std::shared_ptr<UCollisionComponent> Component;
        size_t TreeNodeId;
    };

public:
    static UCollisionManager* Get() {
        static UCollisionManager Instance;
        if (!Instance.bIsInitialized)
        {
            Instance.Initialize();
        }
        return &Instance;
    }

    void RegisterCollision(std::shared_ptr<UCollisionComponent>& NewComponent,
                              const std::shared_ptr<URigidBodyComponent>& InRigidBody);

    void RegisterCollision(std::shared_ptr<UCollisionComponent>& NewComponent);

    void Tick(const float DeltaTime);
    void UnRegisterAll();
    bool IsInitialized() const { return bIsInitialized; }

    size_t GetRegisterComponentsCount() { return RegisteredComponents.size(); }

public:
    FCollisionSystemConfig Config;

private:
    void Initialize();
    void Release();
    void CleanupDestroyedComponents();
    inline bool IsDestroyedComponent(size_t Idx) const {
        return RegisteredComponents[Idx].Component->bDestroyed;
    }
    //충돌 쌍의 인덱스 새로 업데이트 -for deletion
    void UpdateCollisionPairIndices(size_t OldIndex, size_t NewIndex);

    //검색 헬퍼
    size_t FindComponentIndex(size_t TreeNodeId) const;

    // 충돌 처리 관련 함수들
    void ProcessCollisions(const float DeltaTime);

    //CCD 임계속도 비교
    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    //새로운 충돌쌍 업데이트
    void UpdateCollisionPairs();

    //컴포넌트 트랜스폼 업데이트
    void UpdateCollisionTransform();

    void GetCollisionDetectionParams(const std::shared_ptr<UCollisionComponent>& InComp, FCollisionResponseParameters& Result) const;

    //일반적인 충돌반응 (충격량기반 속도 변화)
    void ApplyCollisionResponseByImpulse(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectResult);

    //연속적인 충돌 반응 (속도 기반)
    void HandlePersistentCollision(
        const FCollisionPair& InPair,
        const FCollisionDetectionResult& DetectResult,
        const float DeltaTime);

    void ApplyPositionCorrection(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectResult,
        const float DeltaTime);

    //제약조건 기반 반복적 해결
    void ApplyCollisionResponseByContraints(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectResult);

    void BroadcastCollisionEvents(
        const FCollisionPair& InPair,
        const FCollisionDetectionResult& DetectResult);


private:
    // 하부 시스템 클래스들
    class FCollisionDetector* Detector = nullptr;
    class FCollisionResponseCalculator* ResponseCalculator = nullptr;
    class FCollisionEventDispatcher* EventDispatcher = nullptr;

    // 컴포넌트 관리
    std::vector<FComponentData> RegisteredComponents; //순차 접근-캐시효율성 을 위한 벡터 사용
    FDynamicAABBTree* CollisionTree = nullptr;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;

    bool bIsInitialized = false;
};