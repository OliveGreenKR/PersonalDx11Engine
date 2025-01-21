#pragma once
#include "GameObject.h"
#include "Math.h"

struct FTransform;

class UCamera : public UGameObject
{
public:
	UCamera();
	~UCamera() override;

	Matrix GetViewMatrix();
	Matrix GetProjectionMatrix();

	void SetFov(const float InFov) { Fov = InFov;  bUpdateNeeded = true; }
	void SetAspectRatio(float InRatio) { AspectRatio = InRatio; bUpdateNeeded = true;}
	void SetNearZ(float InZ) { NearZ = InZ; bUpdateNeeded = true; }
	void SetFarZ(float InZ) { FarZ = InZ; bUpdateNeeded = true; }

	void SetProjectionParameters(float InFov, float InAspectRatio,float InNearZ, float InFarZ);

private :

	float bUpdateNeeded = true;
	//radian
	float Fov = PI/2.0f; //90 degree
	float AspectRatio = 16.0f / 9.0f;
	float NearZ = 0.1f;
	float FarZ = 1.0f;

	Matrix ViewMatrix = XMMatrixIdentity();
	Matrix ProjectionMatrix = XMMatrixIdentity();
};

