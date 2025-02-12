#pragma once
#include "Math.h"
#include "DynamicAABBTree.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include <memory>
#include <vector>
#include <unordered_set>

class UCollisionComponent;
class URigidBodyComponent;

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
    float MinimumTimeStep = 0.0016f;     // �ּ� �ð� ���� (�� 600fps)
    float MaximumTimeStep = 0.0166f;     // �ִ� �ð� ���� (�� 60fps)
    float CCDMotionThreshold = 1.0f;     // CCD Ȱ��ȭ �ӵ� �Ӱ谪

    // AABB Tree ���� ����
    size_t InitialCapacity = 1024;       // �ʱ� ������Ʈ �� Ʈ�� �뷮
    float AABBMargin = 0.1f;             // AABB ���� ����
};

class UCollisionManager
{
private:
    // ���� �� �̵� ����
    UCollisionManager(const UCollisionManager&) = delete;
    UCollisionManager& operator=(const UCollisionManager&) = delete;
    UCollisionManager(UCollisionManager&&) = delete;
    UCollisionManager& operator=(UCollisionManager&&) = delete;

    // ������/�Ҹ���
    UCollisionManager() = default;
    ~UCollisionManager();

private:
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
    //�浹 ���� �ε��� ���� ������Ʈ
    void UpdateCollisionPairIndices(size_t OldIndex, size_t NewIndex);
    UCollisionComponent* FindComponentByTreeNodeId(size_t TreeNodeId) const;
    size_t FindComponentIndex(size_t TreeNodeId) const;

private:
    // �浹 ó�� ���� �Լ���
    void ProcessCollisions(const float DeltaTime);

    void UpdateCollisionPairs();

    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    void ProcessCCDCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        float DeltaTime);

    void HandleCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);

    void ApplyCollisionResponse(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult);

    void BroadcastCollisionEvents(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);

private:
    // �Ϻ� �ý��� Ŭ������
    FCollisionDetector* Detector = nullptr;
    FCollisionResponseCalculator* ResponseCalculator = nullptr;
    FCollisionEventDispatcher* EventDispatcher = nullptr;

    // ������Ʈ ����
    std::vector<FComponentData> RegisteredComponents;
    FDynamicAABBTree* CollisionTree = nullptr;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;

    bool bIsInitialized = false;
};