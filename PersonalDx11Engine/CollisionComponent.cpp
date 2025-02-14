#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"



UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	RigidBody = InRigidBody;
}

UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InShape, const Vector3& InHalfExtents)
{
	RigidBody = InRigidBody;
	Shape.Type = InShape;
	Shape.HalfExtent = InHalfExtents;
}

UCollisionComponent::UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents)
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
	if (RigidBody.lock())
	{
		auto OwnerPtr = RigidBody.lock()->GetOwner();
		return RigidBody.lock()->GetOwner()->GetTransform();
	}
		

	return &PrevTransform;
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
		RigidBody.lock()->AddChild(shared_from_this());
	}
}

void UCollisionComponent::OnOwnerTransformChanged(const FTransform& InChanged)
{
	bIsTransformDirty = true;
}


void UCollisionComponent::PostInitialized()
{
	UActorComponent::PostInitialized();

	if (auto RigidPtr = RigidBody.lock())
	{
		GetOwner()->GetTransform()->
			OnTransformChangedDelegate.Bind(shared_from_this(), &UCollisionComponent::OnOwnerTransformChanged, "OnOwnerTransformChanged");
	}
	
}

void UCollisionComponent::Tick(const float DeltaTime)
{
	//test
	if (bIsTransformDirty)
	{
		GetOwner()->SetDebugColor(Color::Red());
	}
	else
	{
		GetOwner()->SetDebugColor(Color::White());
	}
	
}

