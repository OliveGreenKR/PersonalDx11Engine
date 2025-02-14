#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"



//UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
//{
//	RigidBody = InRigidBody;
//}
//
//UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InShape, const Vector3& InHalfExtents)
//{
//	RigidBody = InRigidBody;
//	
//}

UCollisionComponent::UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents) : bDestroyed(false), bCollisionEnabled(true)
{
	Shape.Type = InShape;
	Shape.HalfExtent = InHalfExtents;
}

Vector3 UCollisionComponent::GetHalfExtent() const
{
	return Shape.HalfExtent;
}

const FTransform* UCollisionComponent::GetTransform() const
{
	return GetOwner()->GetTransform();
}

bool UCollisionComponent::IsStatic() const
{
	return RigidBody.lock()->IsStatic();
}


void UCollisionComponent::BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (!InRigidBody.get())
		return;

	RigidBody = InRigidBody;

	if (auto RigidPtr = RigidBody.lock())
	{
		RigidPtr->AddChild(shared_from_this());
	}
}

void UCollisionComponent::OnOwnerTransformChanged(const FTransform& InChanged)
{
	bIsTransformDirty = true;
}


void UCollisionComponent::PostOwnerInitialized()
{
	UActorComponent::PostOwnerInitialized();
}

void UCollisionComponent::PostTreeInitialized()
{
	if (auto RigidPtr = RigidBody.lock())
	{
		GetOwner()->GetTransform()->
			OnTransformChangedDelegate.Bind(shared_from_this(), &UCollisionComponent::OnOwnerTransformChanged, "OnOwnerTransformChanged");
	}
}

void UCollisionComponent::Tick(const float DeltaTime)
{
	UActorComponent::Tick(DeltaTime);

	//test
	if (bIsTransformDirty)
	{
		GetOwner()->SetDebugColor(Color::Red());
	}
	else
	{
		GetOwner()->SetDebugColor(Color::White());
	}

	//이전 트랜스폼 저장
	PrevTransform = *GetTransform();
}

