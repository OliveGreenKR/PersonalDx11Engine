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
	assert(RigidBody.lock());
	return RigidBody.lock()->GetOwner()->GetTransform();
}

void UCollisionComponent::BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (InRigidBody.get())
	{
		RigidBody = InRigidBody;
	}
}

