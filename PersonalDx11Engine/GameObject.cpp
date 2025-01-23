#include "GameObject.h"
#include "Model.h"


void UGameObject::SetPosition(const Vector3& InPosition)
{
	Transform.Position = InPosition;
}

void UGameObject::SetRotation(const Vector3& InRotation)
{
	Transform.Rotation = InRotation;
}

void UGameObject::SetScale(const Vector3& InScale)
{
	Transform.Scale = InScale;
}

void UGameObject::AddPosition(const Vector3& InDelta)
{
	//Transform.Position += InDelta;
}

void UGameObject::AddRotation(const Vector3& InDelta)
{
}

UModel* UGameObject::GetModel() const
{
	if (auto ptr = Model.lock())
	{
		return ptr.get();
	}
	return nullptr;
}

