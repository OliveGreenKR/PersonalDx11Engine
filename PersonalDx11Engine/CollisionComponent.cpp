#include "CollisionComponent.h"
#include "GameObject.h"


UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	Initialize(InRigidBody);
}

UCollisionComponent::UCollisionComponent(const std::shared_ptr<URigidBodyComponent>& InRigidBody, const ECollisionShapeType& InType, const Vector3& InHalfExtents)
{
	Initialize(InRigidBody);
	Shape.Type = InType;
	Shape.HalfExtent = InHalfExtents;
}


void UCollisionComponent::Initialize(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (InRigidBody.get())
	{
		RigidBody = InRigidBody;
	}
}
