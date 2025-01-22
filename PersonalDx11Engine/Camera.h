#pragma once
#include "GameObject.h"
#include "Math.h"

struct FTransform;

class UCamera : public UGameObject
{
public:
	UCamera();
	~UCamera() override = default;

	Matrix GetViewMatrix() const;
	Matrix GetProjectionMatrix() const { return ProjectionMatrix; }

	void SetFov(const float InFov) { Fov = InFov; UpdateProjectionMatrix();}
	void SetAspectRatio(float InRatio) { AspectRatio = InRatio; UpdateProjectionMatrix(); }
	void SetNearZ(float InZ) { NearZ = InZ; UpdateProjectionMatrix();  }
	void SetFarZ(float InZ) { FarZ = InZ; UpdateProjectionMatrix(); }

	void SetProjectionParameters(float InFov, float InAspectRatio,float InNearZ, float InFarZ);
	void UpdateProjectionMatrix() { ProjectionMatrix = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ); }

private :
	//radian
	float Fov = PI/2.0f; //90 degree
	float AspectRatio = 16.0f / 9.0f;
	float NearZ = 0.1f;
	float FarZ = 1.0f;

	Matrix ProjectionMatrix;
};

