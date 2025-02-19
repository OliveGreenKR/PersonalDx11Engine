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

	bool GetSimulatePhysics() const		{ return bSimulatePhysics; }
	bool GetCollisionEnabled() const	{ return bCollisionEnabled; }
};