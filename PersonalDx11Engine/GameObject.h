#pragma once
#include "Transform.h"
#include <memory>

class UModel;

using namespace std;

class UGameObject
{
public:
	UGameObject() = default;
	UGameObject(const shared_ptr<UModel>& InModel) : Model(InModel) {}
	virtual ~UGameObject() = default;

public:
	const FTransform& GetTransform() const { return Transform; }
	FTransform& GetTransform() { return Transform; }
	Matrix GetWorldMatrix() { return Transform.GetModelingMatrix(); }

	void SetModel(const std::shared_ptr<UModel>& InModel) { Model = InModel; }
	UModel* GetModel() const;

protected: 
	FTransform Transform;
	std::weak_ptr<UModel> Model;

};

