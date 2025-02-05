#pragma once
#include <memory>

class UGameObejct;

class IObjectComponent
{
public:
	virtual void AttachTo(std::shared_ptr<class UGameObject>& InOwner) = 0;
	virtual void Tick(const float DeltaTime) = 0;
};