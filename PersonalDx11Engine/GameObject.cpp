#include "GameObject.h"
#include "Model.h"

UGameObject::~UGameObject()
{
	if (Model)
	{
		Model = nullptr;
	}
}

