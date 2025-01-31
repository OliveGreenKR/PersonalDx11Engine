#pragma once
#include <memory>

class UGameObject;
class IObejctCompoenent
{
public:
	virtual void Tick(float DeltaTime) {
		if (bIsTicked == false)
			return;
	}


	bool bIsTicked = false;

protected:
	std::weak_ptr<class UGameObject> Owner;
};