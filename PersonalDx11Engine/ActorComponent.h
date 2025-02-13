#pragma once

class UActorComponent 
{
public:
	virtual void PostInitialized() {}
	virtual void Tick(const float DeltaTime) {}

	virtual const class UGameObject* GetOwner() = 0;
	virtual const UActorComponent* GetOwnerComponent() = 0;
};