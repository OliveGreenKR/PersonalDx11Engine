#pragma once
#include "SceneCompoent.h"

class UPrimitiveComponent : public USceneComponent
{
private:
	bool bSimulatePhysics = false;
	bool bCollisionEnabled = false;

public:
	virtual void Tick(const float DeltaTime) override
	{
		USceneComponent::Tick(DeltaTime);
	}
	void SetSimulatePhysics(const bool bSimulate) { bSimulatePhysics = bSimulate; }
	void SetCollisionEnabled(const bool bEnabled) { bCollisionEnabled = bEnabled; }

	bool IsSimulatePhysics() const		{ return bSimulatePhysics; }
	bool IsCollisionEnabled() const	    { return bCollisionEnabled; }

public:
	virtual void Activate() override
	{ 
		USceneComponent::Activate(); 
		SetSimulatePhysics(false);
		SetCollisionEnabled(false);
	}

	virtual void DeActivate() override
	{
		USceneComponent::DeActivate();
		SetSimulatePhysics(true);
		SetCollisionEnabled(true);
	}
};