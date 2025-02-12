#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "DynamicBoundableInterface.h"


#define OUT

class URigidBodyComponent;
class UGameObject;

// �浹 ���信 �ʿ��� �Ӽ� ����
class UCollisionComponent : public IDynamicBoundable, public std::enable_shared_from_this<UCollisionComponent>
{
public:
    UCollisionComponent() = default;
    UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
    UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InShape, const Vector3& InHalfExtents);
    ~UCollisionComponent() = default;
public:
    // Inherited via IDynamicBoundable
    Vector3 GetHalfExtent() const override;
    const FTransform* GetTransform() const override;
    bool HasBoundsChanged() const override;
    void SetBoundsChanged(bool InBool) override { bBoundsDirty = InBool; }

public:
    // �ʱ�ȭ
    void Initialize();


    // �浹 �̺�Ʈ 
    template<typename T>
    void BindOnCollisionEnter(const std::shared_ptr<T>& InObject,
                              const std::function<void(const FCollisionEventData&)>& InFunction,
                              const std::string& InFunctionName) {
        OnCollisionEnter.Bind(InObject, InFunction, InFunctionName);
    }

    template<typename T>
    void BindOnCollisionStay(const std::shared_ptr<T>& InObject,
                             const std::function<void(const FCollisionEventData&)>& InFunction,
                             const std::string& InFunctionName) {
        OnCollisionStay.Bind(InObject, InFunction, InFunctionName);
    }

    template<typename T>
    void BindOnCollisionExit(const std::shared_ptr<T>& InObject,
                             const std::function<void(const FCollisionEventData&)>& InFunction,
                             const std::string& InFunctionName) {
        OnCollisionExit.Bind(InObject, InFunction, InFunctionName);
    }

public:
    URigidBodyComponent* GetRigidBody() const { return RigidBody.lock().get(); }

    void SetCollisionShape(const FCollisionShapeData& InShape) {
        Shape = InShape; 
        bBoundsDirty = true;
    }
    const FCollisionShapeData& GetCollisionShape() const { return Shape; }
    //FCollisionShapeData& GetCollisionShape() { return Shape; }

    const Vector3& GetPreviousPosition() const { return PreviousPosition; }
    void SetPreviousPosition(const Vector3& InPosition) { PreviousPosition = InPosition; }

public:
    bool bCollisionEnabled = true;
    bool bDestroyed = false;
    
public:
    bool bBoundsDirty = false;
public:
    void OnOwnerTransformChagned() { bBoundsDirty = true; }
public:
    // �浹 �̺�Ʈ publish
    void OnCollisionEnterEvent(const FCollisionEventData& CollisionInfo) {
        OnCollisionEnter.Broadcast(CollisionInfo);
    }

    void OnCollisionStayEvent(const FCollisionEventData& CollisionInfo) {
        OnCollisionStay.Broadcast(CollisionInfo);
    }

    void OnCollisionExitEvent(const FCollisionEventData& CollisionInfo) {
        OnCollisionExit.Broadcast(CollisionInfo);
    }

private:
    std::weak_ptr<URigidBodyComponent> RigidBody;
    FCollisionShapeData Shape;
    Vector3 PreviousPosition;    // CCD�� ���� ���� ������ ��ġ


    // �浹 �̺�Ʈ ��������Ʈ
    FDelegate<const FCollisionEventData&> OnCollisionEnter;
    FDelegate<const FCollisionEventData&> OnCollisionStay;
    FDelegate<const FCollisionEventData&> OnCollisionExit;

    friend class UCollisionManager;

};