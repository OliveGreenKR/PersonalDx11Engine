#pragma once
#include "GameObject.h"
#include "Frustum.h"

class UCamera : public UGameObject
{
public:
	UCamera();
	UCamera(float fov, float aspectRatio, float nearZ, float farZ);
	~UCamera() override = default;
public:
	virtual void Tick(const float DeltaTime) override;

public:
	Matrix GetViewMatrix() const;
	Matrix GetProjectionMatrix() const;

	void SetFov(const float InFov) { Fov = InFov; UpdateProjectionMatrix();}
	void SetAspectRatio(float InRatio) { AspectRatio = InRatio; UpdateProjectionMatrix(); }
	void SetNearZ(float InZ) { NearZ = InZ; UpdateProjectionMatrix();  }
	void SetFarZ(float InZ) { FarZ = InZ; UpdateProjectionMatrix(); }

	bool IsInView(const Vector3& Position);
	void SetLookAtObject(shared_ptr<UGameObject>& InTarget);

	void LookTo(const Vector3& TargetPosition);

protected:
	void UpdateToLookAtObject(float DeltaTime);

public:
	bool bTrackObject = false;
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

	Vector3 Up = Vector3::Up;
	Vector3 LookAt = Vector3::Forward;

	mutable bool bIsViewDirty = false;
	mutable Matrix ViewMatrix;
	mutable FFrustum ViewFrustum;

#pragma region CameraFollow
public:
private:
	//Cameara Follw to Object(Only Rotate)
	//TODO : Camera Follow with Position? -> 'Camera Spring Arm'
	weak_ptr<UGameObject> LookAtObject;

	// 회전 보간 속도 제어
	float RotationDampingSpeed = 5.0f;

	// 거리에 따른 속도 조절을 위한 변수들
	float MinRotationSpeed = 1.0f;
	float MaxRotationSpeed = 10.0f;
	float DistanceSpeedScale = 1.0f;

	// 최소/최대 추적 각도
	float MinTrackingAngle = .0f;
	float MaxTrackingAngle = 30.0f;
#pragma endregion
};
