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

#pragma endregion

#pragma region ICollisionShape Implementation

Vector3 UCollisionComponentBase::GetHalfExtent() const
{
	return Vector3(0.5f, 0.5f, 0.5f);
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

#pragma region SceneComponent Overrides

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

#pragma endregion