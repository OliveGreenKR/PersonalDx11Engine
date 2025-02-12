#include "CollisionComponent.h"
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


void UCollisionComponent::Initialize()
{
	if (auto RigidPtr = RigidBody.lock())
	{
		RigidPtr->GetOwner()->OnTransformChangedDelegate.Bind(shared_from_this(), [this]() {OnOwnerTransformChagned();}, "BoundsCheck");
	}
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

bool UCollisionComponent::HasBoundsChanged() const
{
	return bBoundsDirty;
}
