#pragma once
#include "ActorComponent.h"
#include "Transform.h"

class USceneComponent : public UActorComponent
{
protected:
	FTransform ComponentTransform;

public:
	virtual void Tick(const float DeltaTime) override
	{
		UActorComponent::Tick(DeltaTime);
	}
	virtual const FTransform* GetTransform() const { return &ComponentTransform; }
	virtual FTransform* GetTransform() { return &ComponentTransform; }
};
