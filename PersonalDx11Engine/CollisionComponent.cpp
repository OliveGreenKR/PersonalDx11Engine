#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"
#include "CollisionManager.h"


UCollisionComponent::UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents) : bDestroyed(false)
{
	Shape.Type = InShape;
	Shape.HalfExtent = InHalfExtents;
}

UCollisionComponent::UCollisionComponent() :  bDestroyed(false)
{
}

Vector3 UCollisionComponent::GetHalfExtent() const
{
	return Shape.HalfExtent;
}

const FTransform* UCollisionComponent::GetTransform() const
{
	auto RigidPtr = RigidBody.lock();

	if (RigidPtr)
	{
		return RigidPtr->GetTransform();
	}
	return &LocalTransform;
}

FTransform* UCollisionComponent::GetTransform()
{
	auto RigidPtr = RigidBody.lock();

	if (RigidPtr)
	{
		return RigidPtr->GetTransform();
	}
	return &LocalTransform;
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
		Vector3 NewInerteria = CalculateRotationalInerteria(RigidPtr->GetMass());
		RigidPtr->SetRotationalInertia(NewInerteria, URigidBodyComponent::RotationalInertiaToken());
	}
}

void UCollisionComponent::SetCollisionShapeData(const FCollisionShapeData& InShape)
{
	Shape = InShape;

	if (auto RigidPtr = RigidBody.lock())
	{
		Vector3 NewInerteria = CalculateRotationalInerteria(RigidPtr->GetMass());
		RigidPtr->SetRotationalInertia(NewInerteria, URigidBodyComponent::RotationalInertiaToken());
	}
	
}

void UCollisionComponent::SetHalfExtent(const Vector3& InHalfExtent)
{
	Shape.HalfExtent = InHalfExtent;
}

void UCollisionComponent::Activate()
{
	UPrimitiveComponent::Activate();
	ActivateColiision();
}

void UCollisionComponent::DeActivate()
{
	USceneComponent::DeActivate();
	DeActivateCollision();
}

void UCollisionComponent::SetShape(const ECollisionShapeType InShape)
{
	Shape.Type = InShape;
}

void UCollisionComponent::SetShapeSphere()
{
	Shape.Type = ECollisionShapeType::Sphere;
}

void UCollisionComponent::SetShapeBox()
{
	Shape.Type = ECollisionShapeType::Box;
}

void UCollisionComponent::ActivateColiision()
{
	auto shared = std::dynamic_pointer_cast<UCollisionComponent>(shared_from_this());
	UCollisionManager::Get()->RegisterCollision(shared);
}

void UCollisionComponent::DeActivateCollision()
{
	auto shared = std::dynamic_pointer_cast<UCollisionComponent>(shared_from_this());
	UCollisionManager::Get()->UnRegisterCollision(shared);
}

void UCollisionComponent::OnOwnerTransformChanged(const FTransform& InChanged)
{
	bIsTransformDirty = true;
}

Vector3 UCollisionComponent::CalculateRotationalInerteria(const float InMass)
{

	switch (Shape.Type)
	{
		case ECollisionShapeType::Box : 
		{
			Vector3 Result;
			Result.x = (1.0f / 12.0f) * InMass * (Shape.HalfExtent.y * 4 + Shape.HalfExtent.z * 4);
			Result.y = (1.0f / 12.0f) * InMass * (Shape.HalfExtent.x * 4 + Shape.HalfExtent.z * 4);
			Result.z = (1.0f / 12.0f) * InMass * (Shape.HalfExtent.x * 4 + Shape.HalfExtent.y * 4);
			return Result;
		}
		break;
		case ECollisionShapeType::Sphere : 
		{
			float radius = Shape.GetSphereRadius();
			return InMass * (0.4f) * radius * radius * Vector3::One;
		}
		break;
		default :
		{
			return InMass * 5.0f * Vector3::One;
		}
		break;
	}
}

const FTransform& UCollisionComponent::GetWorldTransform() const
{
	return USceneComponent::GetWorldTransform();
}

void UCollisionComponent::PostTreeInitialized()
{
	USceneComponent::PostTreeInitialized();
}

void UCollisionComponent::Tick(const float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);

	if (!IsActive())
		return;

	//이전 트랜스폼 저장
	PrevWorldTransform = GetWorldTransform();
}

