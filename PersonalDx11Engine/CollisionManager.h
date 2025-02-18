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
    bool bPrevCollided : 1;

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
    float CCDVelocityThreshold = 3.0f;     // CCD Ȱ��ȭ �ӵ� �Ӱ谪

    // AABB Tree ���� ����
    size_t InitialCapacity = 1024;       // �ʱ� ������Ʈ �� Ʈ�� �뷮
    float AABBMargin = 0.1f;             // AABB ���� ����
};

/// <summary>
/// ��ϵ� �����͵��� �浹 ������ ����
/// DynamicAABBTree�� �̿��� ��ü�� �浹���� �����ϰ�
/// �浹 �׽�Ʈ �� �浹 �������� ������
/// </summary>
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
    //������Ʈ ���� ����ü
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
    //�浹 ���� �ε��� ���� ������Ʈ -for deletion
    void UpdateCollisionPairIndices(size_t OldIndex, size_t NewIndex);

    //�˻� ����
    size_t FindComponentIndex(size_t TreeNodeId) const;

    // �浹 ó�� ���� �Լ���
    void ProcessCollisions(const float DeltaTime);

    //CCD �Ӱ�ӵ� ��
    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    //���ο� �浹�� ������Ʈ
    void UpdateCollisionPairs();

    //������Ʈ Ʈ������ ������Ʈ
    void UpdateCollisionTransform();

    FCollisionDetectionResult DetectCCDCollision(
        const FCollisionPair& InPair,
        const float DeltaTime);

    FCollisionDetectionResult DetectDCDCollision(
        const FCollisionPair& InPair,
        const float DeltaTime);

    void GetCollisionDetectionParams(const std::shared_ptr<UCollisionComponent>& InComp, FCollisionResponseParameters& Result) const;

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
        const FCollisionPair& InPair,
        const FCollisionDetectionResult& DetectionResult);

private:
    // �Ϻ� �ý��� Ŭ������
    class FCollisionDetector* Detector = nullptr;
    class FCollisionResponseCalculator* ResponseCalculator = nullptr;
    class FCollisionEventDispatcher* EventDispatcher = nullptr;

    // ������Ʈ ����
    std::vector<FComponentData> RegisteredComponents; //���� ����-ĳ��ȿ���� �� ���� ���� ���
    FDynamicAABBTree* CollisionTree = nullptr;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;

    bool bIsInitialized = false;
};