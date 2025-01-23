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
	void SetPosition(const Vector3& InPosition);
	//{ Pitch, Yaw, Roll }
	void SetRotation(const Vector3& InRotation);
	void SetScale(const Vector3& InScale);

	void AddPosition(const Vector3& InDelta);
	//{ Pitch, Yaw, Roll }
	void AddRotation(const Vector3& InDelta);

	__forceinline const FTransform& GetTransform() const { return Transform; }
	
	Matrix GetWorldMatrix() const  { return Transform.GetModelingMatrix(); }

	void SetModel(const std::shared_ptr<UModel>& InModel) { Model = InModel; }
	UModel* GetModel() const;
protected:
	virtual void OnTransformChanged() {};

protected:

	FTransform Transform;
	std::weak_ptr<UModel> Model;

};

