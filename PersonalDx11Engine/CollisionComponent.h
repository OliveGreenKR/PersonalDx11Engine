#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "DynamicBoundableInterface.h"

class URigidBodyComponent;
class UGameObject;

// 충돌 응답에 필요한 속성 관리
class UCollisionComponent : public IDynamicBoundable
{
    friend class UCollisionManager;
public:
    UCollisionComponent() = default;
    UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
    UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InShape, const Vector3& InHalfExtents);
    ~UCollisionComponent() = default;
public:
    // Inherited via IDynamicBoundable
    Vector3 GetHalfExtent() const override;
    const FTransform* GetTransform() const override;
    bool IsStatic() const override;

public:
    void UpdatePrevTransform();

    // 초기화
    void BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody);


    // 충돌 이벤트 
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

    void SetCollisionShape(const FCollisionShapeData& InShape) { Shape = InShape; }
    const FCollisionShapeData& GetCollisionShape() const { return Shape; }

    const FTransform& GetPreviousTransform() const { return PrevTransform; }

public:
    bool bCollisionEnabled = true;
    bool bDestroyed = false;

public:
    // 충돌 이벤트 publish
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
    FTransform PrevTransform;    // CCD를 위한 이전 프레임 위치


    // 충돌 이벤트 델리게이트
    FDelegate<const FCollisionEventData&> OnCollisionEnter;
    FDelegate<const FCollisionEventData&> OnCollisionStay;
    FDelegate<const FCollisionEventData&> OnCollisionExit;

};