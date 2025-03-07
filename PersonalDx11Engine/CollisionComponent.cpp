#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"
#include "CollisionManager.h"


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
	auto RigidPtr = RigidBody.lock();

	if (RigidPtr)
	{
		return RigidPtr->GetTransform();
	}
	return &ComponentTransform;
}

FTransform* UCollisionComponent::GetTransform()
{
	auto RigidPtr = RigidBody.lock();

	if (RigidPtr)
	{
		return RigidPtr->GetTransform();
	}
	return &ComponentTransform;
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

bool UCollisionComponent::IsEffective()
{
	bool result = UPrimitiveComponent::IsEffective();
	return  result && !bDestroyed && IsCollisionEnabled();
}

void UCollisionComponent::Activate()
{
	UPrimitiveComponent::Activate();
	ActivateColiision();
}

void UCollisionComponent::DeActivate()
{
	UPrimitiveComponent::DeActivate();
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

void UCollisionComponent::PostTreeInitialized()
{
	Activate();

	if (auto RigidPtr = RigidBody.lock())
	{
		GetOwner()->GetTransform()->
			OnTransformChangedDelegate.Bind(shared_from_this(), &UCollisionComponent::OnOwnerTransformChanged, "OnOwnerTransformChanged");
	}
}

void UCollisionComponent::Tick(const float DeltaTime)
{
	UActorComponent::Tick(DeltaTime);

	if (!IsActive() && !IsCollisionEnabled())
		return;

	//���� Ʈ������ ����
	PrevTransform = *GetTransform();
}

