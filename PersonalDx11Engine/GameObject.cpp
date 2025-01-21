#include "GameObject.h"
#include "Model.h"

UGameObject::~UGameObject()
{
	if (Model)
	{
		Model->Release();
		Model = nullptr;
	}
}

