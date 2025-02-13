#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"



UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	BindRigidBody(InRigidBody);
}

UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InShape, const Vector3& InHalfExtents)
{
	BindRigidBody(InRigidBody);
	Shape.Type = InShape;
	Shape.HalfExtent = InHalfExtents;
}

Vector3 UCollisionComponent::GetHalfExtent() const
{
	return Shape.HalfExtent;
}

const FTransform* UCollisionComponent::GetTransform() const
{
	if(RigidBody.lock())
		return RigidBody.lock()->GetOwner()->GetTransform();

	return nullptr;
}

bool UCollisionComponent::IsStatic() const
{
	return RigidBody.lock()->IsStatic();
}


void UCollisionComponent::BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (InRigidBody.get())
	{
		RigidBody = InRigidBody;
	}
}

void UCollisionComponent::OnOwnerTransformChanged(const FTransform& InChanged)
{
	bIsTransformDirty = true;

	//test
	GetOwner()->SetDebugColor(Color::Red());
}


void UCollisionComponent::PostInitialized()
{
	UActorComponent::PostInitialized();
	GetOwner()->GetTransform()->
		OnTransformChangedDelegate.Bind(shared_from_this(), &UCollisionComponent::OnOwnerTransformChanged, "OnOwnerTransformChanged");
}

