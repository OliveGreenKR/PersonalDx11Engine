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
        : IndexA(InIndexA < InIndexB ? IndexA : IndexB)
        , IndexB(InIndexA >= InIndexB ? IndexA : IndexB)
    {
    }

    size_t IndexA; //always smaller than indexB
    size_t IndexB; //always smaller than indexA

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

    // 팩토리 메소드: 외부에서 사용할 컴포넌트 생성
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
    // 시스템 설정
    FCollisionSystemConfig Config;

private:
    void Initialize();
    void Release();

    // 실제 컴포넌트 삭제 처리
    void CleanupDestroyedComponents();
    inline bool IsDestroyedComponent(size_t idx) { return RegisteredComponents[idx]->bDestroyed; }

private:
    // 충돌 검사 및 응답 처리를 위한 내부 함수들
    void ProcessCollisions(float DeltaTime);
    void UpdateCollisionPairs();
    void CleanupDestroyedComponents();

    // CCD 관련 함수들
    bool ShouldUseCCD(const URigidBodyComponent* RigidBody) const;

    void ProcessCCDCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        float DeltaTime);

    // 실제 충돌 처리 및 이벤트 발행
    void HandleCollision(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);

    void ApplyCollisionResponse(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult);

    // 이벤트 처리
    void BroadcastCollisionEvents(
        const std::shared_ptr<UCollisionComponent>& ComponentA,
        const std::shared_ptr<UCollisionComponent>& ComponentB,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);

    // 충돌 상태 관리
    void UpdateCollisionState(
        const FCollisionPair& Pair,
        bool CurrentlyColliding,
        const FCollisionDetectionResult& DetectionResult,
        float DeltaTime);

private:
    FCollisionDetector* Detector;
    FCollisionResponseCalculator* ResponseCalculator;
    FCollisionEventDispatcher* EventDispatcher;

    std::vector<std::shared_ptr<UCollisionComponent>> RegisteredComponents;
    std::unordered_set<FCollisionPair> ActiveCollisionPairs;
};