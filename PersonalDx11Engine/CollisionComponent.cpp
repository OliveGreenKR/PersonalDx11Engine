#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"
#include "CollisionManager.h"
#include "TypeCast.h"


//UCollisionComponentBase::UCollisionComponentBase(const Vector3& InHalfExtents) 
//	: UCollisionComponentBase()
//{
//}

UCollisionComponentBase::UCollisionComponentBase() 
{
}

UCollisionComponentBase::~UCollisionComponentBase()
{
}

Vector3 UCollisionComponentBase::GetHalfExtent() const
{
	return GetLocalTransform().Scale * 0.5f;
}

Vector3 UCollisionComponentBase::GetScaledHalfExtent() const
{
	Vector3 HalfExtent = GetHalfExtent();
	return Vector3(
		HalfExtent.x * GetWorldTransform().Scale.x,
		HalfExtent.y * GetWorldTransform().Scale.y,
		HalfExtent.z * GetWorldTransform().Scale.z);
}

bool UCollisionComponentBase::IsStatic() const
{
	return RigidBody.lock()->IsStatic();
}

void UCollisionComponentBase::BindRigidBody(const std::shared_ptr<URigidBodyComponent>& InRigidBody)
{
	if (!InRigidBody.get())
		return;

	RigidBody = InRigidBody;

	if (auto RigidPtr = RigidBody.lock())
	{
		RigidPtr->AddChild(shared_from_this());
		Vector3 NewInerteria = CalculateInertiaTensor(RigidPtr->GetMass());
		RigidPtr->SetRotationalInertia(NewInerteria, URigidBodyComponent::RotationalInertiaToken());
	}

}

void UCollisionComponentBase::SetHalfExtent(const Vector3& InHalfExtent)
{
	SetLocalScale(InHalfExtent * 2.0f);
}

void UCollisionComponentBase::Activate()
{
	USceneComponent::Activate();
	ActivateColiision();
}

void UCollisionComponentBase::DeActivate()
{
	USceneComponent::DeActivate();
	DeActivateCollision();
}

void UCollisionComponentBase::ActivateColiision()
{
	auto shared = Engine::Cast<UCollisionComponentBase>(shared_from_this());
	if (shared)
	{
		UCollisionManager::Get()->RegisterCollision(shared);
	}
}

void UCollisionComponentBase::DeActivateCollision()
{
	auto shared = Engine::Cast<UCollisionComponentBase>(shared_from_this());
	if (shared) 
	{
		UCollisionManager::Get()->UnRegisterCollision(shared);
	}
}

const FTransform& UCollisionComponentBase::GetWorldTransform() const
{
	return USceneComponent::GetWorldTransform();
}

void UCollisionComponentBase::PostInitialized()
{
	USceneComponent::PostInitialized();
	if (IsActive())
	{

		ActivateColiision();
	}
}

void UCollisionComponentBase::PostTreeInitialized()
{
	USceneComponent::PostTreeInitialized();
	//Rigid가 없다면 부모중에 Rigid 검색
	if (!RigidBody.lock())
	{
		auto Rigid = GetRoot()->FindComponentByType<URigidBodyComponent>();
		if (Rigid.lock())
		{
			auto RigidChild = Rigid.lock()->FindChildByType<UCollisionComponentBase>();
			if (RigidChild.lock() && RigidChild.lock().get() == this)
			{
				BindRigidBody(Rigid.lock());
			}
		}
	}
}

void UCollisionComponentBase::Tick(const float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);

	if (!IsActive())
		return;

	//이전 트랜스폼 저장
	PrevWorldTransform = GetWorldTransform();

	if (bIsDebugVisualize)
	{
		RequestDebugRender(DeltaTime);
	}
}

