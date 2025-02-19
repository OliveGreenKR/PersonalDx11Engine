#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"


UCollisionComponent::UCollisionComponent(const ECollisionShapeType& InShape, const Vector3& InHalfExtents) : bDestroyed(false)
{
	SetCollisionEnabled(true);
	Shape.Type = InShape;
	Shape.HalfExtent = InHalfExtents;
}

UCollisionComponent::UCollisionComponent() :  bDestroyed(false)
{
	SetCollisionEnabled(true);
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
		Vector3 NewInerteria = CalculateRotationalInerteria(RigidPtr->GetMass());
		RigidPtr->SetRotationalInertia(NewInerteria, URigidBodyComponent::RotationalInertiaToken());
	}
}

void UCollisionComponent::SetCollisionShape(const FCollisionShapeData& InShape)
{
	Shape = InShape;

	if (auto RigidPtr = RigidBody.lock())
	{
		Vector3 NewInerteria = CalculateRotationalInerteria(RigidPtr->GetMass());
		RigidPtr->SetRotationalInertia(NewInerteria, URigidBodyComponent::RotationalInertiaToken());
	}
	
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

