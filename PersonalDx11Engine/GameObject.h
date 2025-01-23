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
	virtual void Tick(const float DeltaTime);

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

public:

	void StartMove(const Vector3& InTaget);
	void StopMoveSlowly();
	void StopMoveImmediately();

	void UpdateMovement(const float DeltaTime);
protected:
	void UpdateVelocity(const float DeltaTime);
	void UpdatePosition(const float DeltaTime);

public:
	//movement test
	bool bIsMoving = false;
	bool bIsPhysicsBasedMove = true;

	float Acceleration = 2.0f;
	float Deceleration = 1000.0f;
	float MaxSpeed = 100.0f;

	Vector3 TargetVelocity;
	Vector3 CurrentVelocity;
	Vector3 TargetPosition;
};