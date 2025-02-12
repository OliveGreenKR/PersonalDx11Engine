#pragma once
#include "Math.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "CollisionDetector.h"
#include "CollisionResponseCalculator.h"
#include "CollisionEventDispatcher.h"
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

class UCollisionComponent;
class URigidBodyComponent;

#pragma region CollisionPair
struct FCollisionPair
{
    FCollisionPair(size_t InIndexA, size_t InIndexB)
        : IndexA(InIndexA < InIndexB ? InIndexA : InIndexB)
        , IndexB(InIndexA >= InIndexB ? InIndexA : InIndexB)
    {}

    FCollisionPair(const FCollisionPair& Other) = default;
    FCollisionPair& operator=(const FCollisionPair& Other) = default;

    size_t IndexA; //always smaller than indexB
    size_t IndexB; //always smaller than indexA}

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
};


class UCollisionManager
{
private:
    UCollisionManager();
    ~UCollisionManager();

public:
    static UCollisionManager* Get() {
        static UCollisionManager Instance;
        return &Instance;
    }

    // ���丮 �޼ҵ�: �ܺο��� ����� ������Ʈ ����
    std::shared_ptr<UCollisionComponent> Create(
        const std::shared_ptr<URigidBodyComponent>& InRigidBody,
        const ECollisionShapeType& InType = ECollisionShapeType::Sphere,
        const Vector3& InHalfExtents = Vector3::One * 0.5f)
    {
        auto NewComponent = std::shared_ptr<UCollisionComponent>(
            new UCollisionComponent(InRigidBody, InType, InHalfExtents)
        );
        RegisteredComponents.push_back(NewComponent);
        return NewComponent;
    }

    void Tick(const float DeltaTime);
    void UnRegisterAll();

public:
    // �ý��� ����
    FCollisionSystemConfig Config;

private:
    void Initialize();
    void Release();

    // ���� ������Ʈ ���� ó��
    void CleanupDestroyedComponents();
    inline bool IsDestroyedComponent(size_t idx) { return RegisteredComponents[idx]->bDestroyed; }

private:
    // �浹 �˻� �� ���� ó���� ���� ���� �Լ���
    void ProcessCollisions(const float DeltaTime);
    void UpdateCollisionPairs();

    // CCD ���� �Լ���
    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    void ProcessCCDCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        float DeltaTime);

    // ���� �浹 ó�� �� �̺�Ʈ ����
    void HandleCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);

    void ApplyCollisionResponse(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult);

    // �̺�Ʈ ó��
    void BroadcastCollisionEvents(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);
    //

private:
    FCollisionDetector* Detector = nullptr;
    FCollisionResponseCalculator* ResponseCalculator = nullptr;
    FCollisionEventDispatcher* EventDispatcher = nullptr;

    std::vector<std::shared_ptr<UCollisionComponent>> RegisteredComponents;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;
};