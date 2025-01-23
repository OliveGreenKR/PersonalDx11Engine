#pragma once
#include "GameObject.h"
#include "Frustum.h"

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

	bool IsInView(const Vector3& Position);
private:
	void OnTransformChanged() override;

private:
	void UpdateProjectionMatrix();

private:
	//logical const
	void UpdatDirtyView() const;
	void UpdateViewMatrix() const;
	void UpdateFrustum() const;
	void CalculateFrustum(Matrix& InViewProj) const;
private :
	//radian
	float Fov = PI/4.0f;		//VerticalFOV
	float AspectRatio = 1.0f;   //affect to HorizontalFOV 
	float NearZ = 0.1f;
	float FarZ = 1.0f;

	Matrix ProjectionMatrix;

	mutable bool bIsViewDirty = false;
	mutable Matrix ViewMatrix;
	mutable FFrustum ViewFrustum;
};
