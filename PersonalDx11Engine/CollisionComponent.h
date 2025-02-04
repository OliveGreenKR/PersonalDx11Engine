#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"

#define OUT

class URigidBodyComponent;
class UGameObject;

enum class ECollisionShapeType
{
    Box,
    Sphere,
};

struct FCollisionResult
{
    struct FCollisionDetectionResult DetectionResult;
    class UCollisionComponent* First = nullptr;
    class UCollisionComponent* Second = nullptr;
};

#pragma region CollsionShape
class FCollisionShape
{
private:
    FCollisionShape() = default;
    ~FCollisionShape() = default;

public:
    // 팩토리
    static FCollisionShape MakeSphere(float Radius) {
        FCollisionShape Shape;
        Shape.Type = ECollisionShapeType::Sphere;
        Shape.HalfExtent = Vector3(Radius, Radius, Radius);
        return Shape;
    }

    static FCollisionShape MakeBox(const Vector3& HalfExtents) {
        FCollisionShape Shape;
        Shape.Type = ECollisionShapeType::Box;
        Shape.HalfExtent = HalfExtents;  // 박스는 각 축별 절반 크기 사용
        return Shape;
    }

    // 충돌 검사 메서드 - CollisionData를 활용
    bool Intersect(const FCollisionShape& Other,
                   const FTransform& ThisTransform,
                   const FTransform& OtherTransform,
                   OUT FCollisionResult& CollisionData) const;

    // AABB 계산
    void GetBoundingBox(OUT Vector3& OutMin, OUT Vector3& OutMax,
                        const FTransform& Transform) const;

    // Getter
    float GetSphereRadius() const {
        assert(Type == ECollisionShapeType::Sphere);
        return HalfExtent.x;  // 구는 x 성분만 사용
    }

    const Vector3& GetBoxHalfExtents() const {
        assert(Type == ECollisionShapeType::Box);
        return HalfExtent;    // 박스는 전체 벡터 사용
    }

private:
    // 충돌 검사 헬퍼 함수들 - CollisionData를 활용
    static bool IntersectSphereSphere(
        const FTransform& TransformA,
        const FTransform& TransformB,
        float RadiusA,
        float RadiusB,
        OUT FCollisionResult& CollisionData);

    static bool IntersectBoxBox(
        const FTransform& TransformA,
        const FTransform& TransformB,
        const Vector3& HalfExtentsA,
        const Vector3& HalfExtentsB,
        OUT FCollisionResult& CollisionData);

    static bool IntersectSphereBox(
        const FTransform& SphereTransform,
        const FTransform& BoxTransform,
        float SphereRadius,
        const Vector3& BoxHalfExtents,
        OUT FCollisionResult& CollisionData);

private:
    ECollisionShapeType Type;
    Vector3 HalfExtent;     
};
#pragma endregion



// 충돌 처리를 담당하는 컴포넌트
class UCollisionComponent
{
public:
    UCollisionComponent() = default;
    ~UCollisionComponent() = default;

    // 초기화
    void Initialize(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
    void SetCollisionShape(const FCollisionShape& InShape);

    // 충돌 이벤트 
    template<typename T>
    void BindOnCollisionEnter(const std::shared_ptr<T>& InObject,
                              const std::function<void(const FCollisionResult&)>& InFunction,
                              const std::string& InFunctionName) {
        OnCollisionEnter.Bind(InObject, InFunction, InFunctionName);
    }

    template<typename T>
    void BindOnCollisionStay(const std::shared_ptr<T>& InObject,
                             const std::function<void(const FCollisionResult&)>& InFunction,
                             const std::string& InFunctionName) {
        OnCollisionStay.Bind(InObject, InFunction, InFunctionName);
    }

    template<typename T>
    void BindOnCollisionExit(const std::shared_ptr<T>& InObject,
                             const std::function<void(const FCollisionResult&)>& InFunction,
                             const std::string& InFunctionName) {
        OnCollisionExit.Bind(InObject, InFunction, InFunctionName);
    }

    // Getter/Setter
    URigidBodyComponent* GetRigidBody() const { return RigidBody.lock().get(); }
    const FCollisionShape& GetCollisionShape() const { return Shape; }
    const Vector3& GetPreviousPosition() const { return PreviousPosition; }
    void SetPreviousPosition(const Vector3& InPosition) { PreviousPosition = InPosition; }

    bool bCollisionEnabled = true;

    // 충돌 이벤트 publish
    void OnCollisionEnterEvent(const FCollisionResult& CollisionInfo) {
        OnCollisionEnter.Broadcast(CollisionInfo);
    }

    void OnCollisionStayEvent(const FCollisionResult& CollisionInfo) {
        OnCollisionStay.Broadcast(CollisionInfo);
    }

    void OnCollisionExitEvent(const FCollisionResult& CollisionInfo) {
        OnCollisionExit.Broadcast(CollisionInfo);
    }

private:
    std::weak_ptr<URigidBodyComponent> RigidBody;
    FCollisionShape Shape;
    Vector3 PreviousPosition;    // CCD를 위한 이전 프레임 위치
  

    // 충돌 이벤트 델리게이트
    FDelegate<const FCollisionResult&> OnCollisionEnter;
    FDelegate<const FCollisionResult&> OnCollisionStay;
    FDelegate<const FCollisionResult&> OnCollisionExit;

    friend class UCollisionManager;
};