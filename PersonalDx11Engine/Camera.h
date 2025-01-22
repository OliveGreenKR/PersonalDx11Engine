#pragma once
#include "GameObject.h"
#include "Math.h"

struct FTransform;

class UCamera : public UGameObject
{
public:
	UCamera();
	UCamera(float fov, float aspectRatio, float nearZ, float farZ);
	~UCamera() override = default;

	Matrix GetViewMatrix() const;
	Matrix GetProjectionMatrix() const;

	void SetFov(const float InFov) { Fov = InFov; UpdateProjectionMatrix();}
	void SetAspectRatio(float InRatio) { AspectRatio = InRatio; UpdateProjectionMatrix(); }
	void SetNearZ(float InZ) { NearZ = InZ; UpdateProjectionMatrix();  }
	void SetFarZ(float InZ) { FarZ = InZ; UpdateProjectionMatrix(); }

private:
	void UpdateProjectionMatrix();
	void UpdateViewMatrix() const;
private :
	//radian
	float Fov = PI/4.0f;		//VerticalFOV
	float AspectRatio = 1.0f;   //affect to HorizontalFOV 
	float NearZ = 0.1f;
	float FarZ = 1.0f;

	Matrix ProjectionMatrix;
	mutable Matrix ViewMatrix;
	mutable bool bisViewDirty = false;

};

