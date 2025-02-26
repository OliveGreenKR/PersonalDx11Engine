#pragma once
#include "Math.h"
#include <memory>
#include "Delegate.h"
#include "Transform.h"
#include "CollisionDefines.h"
#include "DynamicBoundableInterface.h"
#include "PrimitiveComponent.h"

class URigidBodyComponent;
class UGameObject;

// �浹 ���信 �ʿ��� �Ӽ� ����
class UCollisionComponent : public UPrimitiveComponent, public IDynamicBoundable
{
	friend class UCollisionManager;
public:
	UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents);
	~UCollisionComponent() = default;
private:
	UCollisionComponent();
public:
	//Scene Comp
	virtual const FTransform* GetTransform() const override;
	virtual FTransform* GetTransform() override;

	// Inherited via IDynamicBoundable
	Vector3 GetHalfExtent() const override;
	
	bool IsStatic() const override;
	bool IsTransformChanged() const override { return bIsTransformDirty; }
	void SetTransformChagedClean() override { bIsTransformDirty = false; }

protected:
	virtual void PostTreeInitialized() override;
	virtual void Tick(const float DeltaTime) override;

public:
	// �ʱ�ȭ
	void BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody);

public:

	//Getter
	URigidBodyComponent* GetRigidBody() const { return RigidBody.lock().get(); }
	const FCollisionShapeData& GetCollisionShape() const { return Shape; }
	const FTransform& GetPreviousTransform() const { return PrevTransform; }

	//Setter
	void SetCollisionShapeData(const FCollisionShapeData& InShape);
	void SetHalfExtent(const Vector3&& InHalfExtent);

	virtual bool IsEffective() override;
	
public:
	bool bDestroyed : 1;
public:
	// �浹 �̺�Ʈ ��������Ʈ
	FDelegate<const FCollisionEventData&> OnCollisionEnter;
	FDelegate<const FCollisionEventData&> OnCollisionStay;
	FDelegate<const FCollisionEventData&> OnCollisionExit;

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
	void OnOwnerTransformChanged(const FTransform& InChanged);
	Vector3 CalculateRotationalInerteria(const float InMass);

private:
	std::weak_ptr<URigidBodyComponent> RigidBody;
	FCollisionShapeData Shape;
	FTransform PrevTransform = FTransform();    // CCD�� ���� ���� ������ Ʈ������

	bool bIsTransformDirty = false;
}; 