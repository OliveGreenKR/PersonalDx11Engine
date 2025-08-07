#include "CollisionComponent.h"
#include <memory>
#include "GameObject.h"
#include "TypeCast.h"

#pragma region Constructor and Lifecycle

UCollisionComponentBase::UCollisionComponentBase()
{
}

UCollisionComponentBase::~UCollisionComponentBase()
{
}


void UCollisionComponentBase::PostInitialized()
{
	USceneComponent::PostInitialized();
}

void UCollisionComponentBase::PostTreeInitialized()
{
	USceneComponent::PostTreeInitialized();
}

void UCollisionComponentBase::Tick(const float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);

	if (!IsActive())
		return;

	if (bIsDebugVisualize)
	{
		RequestDebugRender(DeltaTime);
	}
}


void UCollisionComponentBase::Activate()
{
	USceneComponent::Activate();
	NotifyActivationChanged(true);
}

void UCollisionComponentBase::DeActivate()
{
	USceneComponent::DeActivate();
	NotifyActivationChanged(false);
}

#pragma endregion

#pragma region Activation State Notification

void UCollisionComponentBase::NotifyActivationChanged(bool bNewActive)
{
	OnActivationChangedDelegate.Broadcast(bNewActive);
}

#pragma endregion

#pragma region ICollisionShape Implementation

Vector3 UCollisionComponentBase::GetLocalHalfExtent() const
{
	return GetLocalTransform().Scale * 0.5f;	
}

Vector3 UCollisionComponentBase::GetScaledHalfExtent() const
{
	return GetWorldTransform().Scale * 0.5f;
}

void UCollisionComponentBase::SetHalfExtent(const Vector3& InHalfExtent)
{
	SetLocalScale(InHalfExtent * 2.0f);
}

#pragma endregion
