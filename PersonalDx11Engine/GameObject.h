#pragma once
#include "Transform.h"
#include <memory>

class UModel;

using namespace std;

class UGameObject
{
	UGameObject() = default;
	UGameObject(shared_ptr<UModel> InModel) : Model(InModel) {}
	virtual ~UGameObject();

public:
	const FTransform& GetTransform() const { return Transform; }
	FTransform& GetTransform() { return Transform; }
	Matrix GetWorldMatrix() { return Transform.GetModelingMatrix(); }

	void SetModel(std::shared_ptr<UModel> InModel) { Model = InModel; }
	std::shared_ptr<UModel> GetModel() const { return Model; }

protected: 
	FTransform Transform;
	std::shared_ptr<UModel> Model = nullptr; 

};

