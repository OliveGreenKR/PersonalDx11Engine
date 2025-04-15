#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "DynamicBoundableInterface.h"
#include "SceneComponent.h"

class URigidBodyComponent;
class UGameObject;

// 충돌 응답에 필요한 속성 관리
class UCollisionComponent : public USceneComponent, public IDynamicBoundable
{
	friend class UCollisionManager;
public:
	UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents);
	UCollisionComponent();

	~UCollisionComponent();
public:

	// Inherited via IDynamicBoundable
	Vector3 GetHalfExtent() const override;
	bool IsStatic() const override;
	const FTransform& GetWorldTransform() const override;

protected:  
	virtual void PostInitialized() override;
	virtual void PostTreeInitialized() override;
	virtual void Tick(const float DeltaTime) override;

public:
	// 초기화
	void BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody);

public:
	//Getter
	URigidBodyComponent* GetRigidBody() const { return RigidBody.lock().get(); }
	const FCollisionShapeData& GetCollisionShape() const { return Shape; }
	const FTransform& GetPreviousTransform() const { return PrevWorldTransform; }

	//Setter
	void SetCollisionShapeData(const FCollisionShapeData& InShape);
	void SetHalfExtent(const Vector3& InHalfExtent);

	//형태 지정
	void SetShape(const ECollisionShapeType InShape);
	void SetShapeSphere();
	void SetShapeBox();
	
private:
	virtual void Activate() override;
	virtual void DeActivate() override;
	void ActivateColiision();
	void DeActivateCollision();
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

	virtual const char* GetComponentClassName() const override { return "UCollision"; }

private:
	Vector3 CalculateRotationalInerteria(const float InMass);

private:
	std::weak_ptr<URigidBodyComponent> RigidBody;
	FCollisionShapeData Shape;
	// CCD를 위한 이전 프레임 월드 트랜스폼
	FTransform PrevWorldTransform = FTransform();
};