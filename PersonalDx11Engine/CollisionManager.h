#pragma once
#include "Math.h"
#include <memory>
#include <vector>
#include <unordered_set>

class FDynamicAABBTree;
class UCollisionComponent;
class URigidBodyComponent;
struct FTransform;

#pragma region CollisionPair
struct FCollisionPair
{
    FCollisionPair(size_t InIndexA, size_t InIndexB)
        : IndexA(InIndexA < InIndexB ? InIndexA : InIndexB)
        , IndexB(InIndexA < InIndexB ? InIndexB : InIndexA)
    {}

    FCollisionPair(const FCollisionPair& Other) = default;
    FCollisionPair& operator=(const FCollisionPair& Other) = default;

    size_t IndexA;
    size_t IndexB;

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

    // AABB Tree 관련 설정
    size_t InitialCapacity = 1024;       // 초기 컴포넌트 및 트리 용량
    float AABBMargin = 0.1f;             // AABB 여유 공간
};

/// <summary>
/// 컴포넌트 콜리전객체의 생성 및 소멸을 관리.
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

    std::shared_ptr<UCollisionComponent> Create(
        const std::shared_ptr<URigidBodyComponent>& InRigidBody,
        const ECollisionShapeType& InType,
        const Vector3& InHalfExtents);

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
    //충돌 쌍의 인덱스 새로 업데이트
    void UpdateCollisionPairIndices(size_t OldIndex, size_t NewIndex);
    UCollisionComponent* FindComponentByTreeNodeId(size_t TreeNodeId) const;
    size_t FindComponentIndex(size_t TreeNodeId) const;

private:
    // 충돌 처리 관련 함수들
    void ProcessCollisions(const float DeltaTime);

    //새로운 충돌쌍 업데이트
    void UpdateCollisionPairs();

    //컴포넌트 트랜스폼 업데이트
    void UpdateCollisionTransform();

    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    FCollisionDetectionResult DetectCCDCollision(
        const FCollisionPair& InPair,
        const float DeltaTime);

    FCollisionDetectionResult DetectDCDCollision(
        const FCollisionPair& InPair,
        const float DeltaTime);

    void HandleCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        const float DeltaTime);

    void ApplyCollisionResponse(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult);

    void BroadcastCollisionEvents(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        const float DeltaTime);

private:
    // 하부 시스템 클래스들
    FCollisionDetector* Detector = nullptr;
    FCollisionResponseCalculator* ResponseCalculator = nullptr;
    FCollisionEventDispatcher* EventDispatcher = nullptr;

    // 컴포넌트 관리
    std::vector<FComponentData> RegisteredComponents; //순차 접근-캐시효율성 을 위한 벡터 사용
    FDynamicAABBTree* CollisionTree = nullptr;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;

    bool bIsInitialized = false;
};