#pragma once
#include "Transform.h"

class UModel;

class UGameObject
{
	UGameObject() = default;
	virtual ~UGameObject();

public:
	const FTransform& GetTransform() const { return Transform; }
	FTransform& GetTransform() { return Transform; }
	Matrix GetWorldMatrix() { return Transform.GetModelingMatrix(); }

protected: 
	FTransform Transform;
	UModel* Model = nullptr;
};

