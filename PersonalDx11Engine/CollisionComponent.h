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
    // ���丮
    static FCollisionShape MakeSphere(float Radius) {
        FCollisionShape Shape;
        Shape.Type = ECollisionShapeType::Sphere;
        Shape.HalfExtent = Vector3(Radius, Radius, Radius);
        return Shape;
    }

    static FCollisionShape MakeBox(const Vector3& HalfExtents) {
        FCollisionShape Shape;
        Shape.Type = ECollisionShapeType::Box;
        Shape.HalfExtent = HalfExtents;  // �ڽ��� �� �ະ ���� ũ�� ���
        return Shape;
    }

    // �浹 �˻� �޼��� - CollisionData�� Ȱ��
    bool Intersect(const FCollisionShape& Other,
                   const FTransform& ThisTransform,
                   const FTransform& OtherTransform,
                   OUT FCollisionResult& CollisionData) const;

    // AABB ���
    void GetBoundingBox(OUT Vector3& OutMin, OUT Vector3& OutMax,
                        const FTransform& Transform) const;

    // Getter
    float GetSphereRadius() const {
        assert(Type == ECollisionShapeType::Sphere);
        return HalfExtent.x;  // ���� x ���и� ���
    }

    const Vector3& GetBoxHalfExtents() const {
        assert(Type == ECollisionShapeType::Box);
        return HalfExtent;    // �ڽ��� ��ü ���� ���
    }

private:
    // �浹 �˻� ���� �Լ��� - CollisionData�� Ȱ��
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



// �浹 ó���� ����ϴ� ������Ʈ
class UCollisionComponent
{
public:
    UCollisionComponent() = default;
    ~UCollisionComponent() = default;

    // �ʱ�ȭ
    void Initialize(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
    void SetCollisionShape(const FCollisionShape& InShape);

    // �浹 �̺�Ʈ 
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

    // �浹 �̺�Ʈ publish
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
    Vector3 PreviousPosition;    // CCD�� ���� ���� ������ ��ġ
  

    // �浹 �̺�Ʈ ��������Ʈ
    FDelegate<const FCollisionResult&> OnCollisionEnter;
    FDelegate<const FCollisionResult&> OnCollisionStay;
    FDelegate<const FCollisionResult&> OnCollisionExit;

    friend class UCollisionManager;
};