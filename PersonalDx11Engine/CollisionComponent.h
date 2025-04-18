#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "DynamicBoundableInterface.h"
#include "SceneComponent.h"
#include "CollisionShapeInterface.h"

class URigidBodyComponent;
class UGameObject;

// 충돌 응답에 필요한 속성을 관리하는 최상위 충돌체 클래스, 직접 사용하지 마시오
class UCollisionComponentBase : public USceneComponent, public IDynamicBoundable, public ICollisionShape
{
	friend class UCollisionManager;
public:
	UCollisionComponentBase(const Vector3& InHalfExtents);
	UCollisionComponentBase();

	~UCollisionComponentBase();
public:

	// Inherited via IDynamicBoundable
	Vector3 GetScaledHalfExtent() const override;
	bool IsStatic() const override;
	const FTransform& GetWorldTransform() const override;

	// Inherited via ICollisionShape
	Vector3 GetHalfExtent() const override;
	void SetHalfExtent(const Vector3& InHalfExtent) override;

	virtual Vector3 GetSupportPoint(const Vector3& Direction, const FTransform& WorldTransform) const  = 0;
	virtual Vector3 CalculateInertiaTensor(float Mass) const = 0;
	virtual void CalculateAABB(const FTransform& WorldTransform, Vector3& OutMin, Vector3& OutMax) const = 0;

	virtual ECollisionShapeType GetType() const override { return ECollisionShapeType::None; }

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
	const FTransform& GetPreviousTransform() const { return PrevWorldTransform; }

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

	virtual const char* GetComponentClassName() const override { return "UCollisionionBase"; }

private:
	std::weak_ptr<URigidBodyComponent> RigidBody;
	// CCD를 위한 이전 프레임 월드 트랜스폼
	FTransform PrevWorldTransform = FTransform();

protected:
	//로컬 extent -  기본값 0.5f
	Vector3 HalfExtent = Vector3::One * 0.5f;
};