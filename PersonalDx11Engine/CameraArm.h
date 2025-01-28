#pragma once
#include "GameObject.h"

class UCameraArm : public UGameObject
{
public:
	UCameraArm() = default;
	virtual ~UCameraArm() override = default;
protected:

	virtual void Tick(float DeltaTime) override;

private:
	weak_ptr<class UCamera> AttachedCamera;
	weak_ptr<class UGameObject> AncheredObject;

};

