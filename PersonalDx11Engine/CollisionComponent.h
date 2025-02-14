#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "DynamicBoundableInterface.h"
#include "ActorComponent.h"

class URigidBodyComponent;
class UGameObject;

// 충돌 응답에 필요한 속성 관리
class UCollisionComponent : public UActorComponent, public IDynamicBoundable
{
    friend class UCollisionManager;
public:
    //UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody);
    //UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InShape, const Vector3& InHalfExtents);
    UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents);
    ~UCollisionComponent() = default;
private:
    UCollisionComponent() = default;
public:
    // Inherited via IDynamicBoundable
    Vector3 GetHalfExtent() const override;
    const FTransform* GetTransform() const override;
    bool IsStatic() const override;
    bool IsTransformChanged() const override { return bIsTransformDirty; }
    void SetTransformChagedClean() override { bIsTransformDirty = false; }

protected:
    virtual void PostInitialized() override;
    virtual void PostTreeInitialized() override;
    virtual void Tick(const float DeltaTime) override;

public:
    // 초기화
    void BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody);

public:
    URigidBodyComponent* GetRigidBody() const { return RigidBody.lock().get(); }

    void SetCollisionShape(const FCollisionShapeData& InShape) { Shape = InShape; }
    const FCollisionShapeData& GetCollisionShape() const { return Shape; }

    const FTransform& GetPreviousTransform() const { return PrevTransform; }

public:
    bool bCollisionEnabled = true;
    bool bDestroyed = false;

public:
    // 충돌 이벤트 델리게이트
    FDelegate<const FCollisionEventData&> OnCollisionEnter;
    FDelegate<const FCollisionEventData&> OnCollisionStay;
    FDelegate<const FCollisionEventData&> OnCollisionExit;

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
    void OnOwnerTransformChanged(const FTransform& InChanged);

private:
    std::weak_ptr<URigidBodyComponent> RigidBody;
    FCollisionShapeData Shape;
    FTransform PrevTransform = FTransform();    // CCD를 위한 이전 프레임 트랜스폼

    bool bIsTransformDirty = false;
};