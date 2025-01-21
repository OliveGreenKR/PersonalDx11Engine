#include "GameObject.h"
#include "Model.h"


UModel* UGameObject::GetModel() const
{
	if (auto ptr = Model.lock())
	{
		return ptr.get();
	}
	return nullptr;
}

