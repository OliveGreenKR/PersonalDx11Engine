#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "RigidBodyComponent.h"
#include"PhysicsSystem.h"
#include "CollisionProcessor.h"
#include "TypeCast.h"
#include "PhysicsStateInternalInterface.h"



UCollisionComponentBase::UCollisionComponentBase() 
{
}

UCollisionComponentBase::~UCollisionComponentBase()
{
}

Vector3 UCollisionComponentBase::GetHalfExtent() const
{
	//Model 1로 정규화되어 있다고 가정
	return Vector3(0.5f, 0.5f, 0.5f);
}

Vector3 UCollisionComponentBase::GetScaledHalfExtent() const
{
	return GetWorldTransform().Scale * 0.5f;
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
		Vector3 NewInvInerteria = CalculateInvInertiaTensor(RigidPtr->GetInvMass());
		RigidPtr->SetInvRotationalInertia(NewInvInerteria);
	}

}

void UCollisionComponentBase::SetHalfExtent(const Vector3& InHalfExtent)
{
	SetLocalScale(InHalfExtent * 2.0f);
}

IPhysicsStateInternal* UCollisionComponentBase::GetPhysicsStateInternal() const
{
	return Engine::Cast<IPhysicsStateInternal>(RigidBody.lock().get());
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
		UPhysicsSystem::GetCollisionSubsystem()->RegisterCollision(shared);
	}
}

void UCollisionComponentBase::DeActivateCollision()
{
	auto shared = Engine::Cast<UCollisionComponentBase>(shared_from_this());
	if (shared) 
	{
		UPhysicsSystem::GetCollisionSubsystem()->UnRegisterCollision(shared);
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

