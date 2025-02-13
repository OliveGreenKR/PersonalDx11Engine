#include "CollisionComponent.h"
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

void UCollisionComponent::UpdatePrevTransform()
{
	if (auto RigidPtr = RigidBody.lock())
	{
		if (auto RigidOwnerTransform = RigidPtr->GetTransform())
		{
			PrevTransform = *RigidOwnerTransform;
		}
	}
}

void UCollisionComponent::BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (InRigidBody.get())
	{
		RigidBody = InRigidBody;
	}
}

