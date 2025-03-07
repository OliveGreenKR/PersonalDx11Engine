﻿#pragma once
#include "GameObject.h"
#include "Frustum.h"
#include "Delegate.h"

class UCamera : public UGameObject
{
public:
	virtual ~UCamera() override = default;

	static std::shared_ptr<UCamera> Create(float fov, float aspectRatio, float nearZ, float farZ)
	{
		return std::shared_ptr<UCamera>(new UCamera(fov,aspectRatio,nearZ,farZ));
	}
protected:
	explicit UCamera(float fov, float aspectRatio, float nearZ, float farZ);
public:
	virtual void Tick(const float DeltaTime) override;
	virtual void PostInitialized() override;

public:
	const Matrix GetViewMatrix();
	const Matrix GetProjectionMatrix() const;

	void SetFov(const float InFov) { Fov = InFov; UpdateProjectionMatrix();}
	void SetAspectRatio(float InRatio) { AspectRatio = InRatio; UpdateProjectionMatrix(); }
	void SetNearZ(float InZ) { NearZ = InZ; UpdateProjectionMatrix();  }
	void SetFarZ(float InZ) { FarZ = InZ; UpdateProjectionMatrix(); }

	bool IsInView(const Vector3& Position);
	void SetLookAtObject(UGameObject* InTarget);

	void LookTo(const Vector3& TargetPosition);
	void LookTo();


public:
	bool bIs2D = false;
	bool bLookAtObject = false;

private:
	void OnTransformChanged(const FTransform& Changed);
	void UpdateProjectionMatrix();

	//logical const
	void UpdatDirtyView();
	void UpdateViewMatrix();
	void UpdateFrustum() const;
	void CalculateFrustum(Matrix& InViewProj) const;

private :
	//radian
	float Fov = PI/4.0f;		//VerticalFOV
	float AspectRatio = 1.0f;   //affect to HorizontalFOV 
	float NearZ = 0.1f;
	float FarZ = 1000.0f;

	Matrix ProjectionMatrix;

	mutable bool bIsViewDirty = true; //첫 일회 갱신필요
	mutable Matrix ViewMatrix;
	mutable FFrustum ViewFrustum;

#pragma region CameraFollow(Rotataion)
protected:
	void UpdateToLookAtObject(const float DeltaTime);
private:
	//Cameara Follw to Object(Only Rotate)
	//TODO : Camera Follow with Position? -> 'Camera Spring Arm'
	weak_ptr<UGameObject> LookAtObject;

	// 회전 보간 속도 제어
	float RotationDampingSpeed = 5.0f;

	// 거리에 따른 속도 조절을 위한 변수들
	float MaxRotationSpeed = 50.0f;
	float DistanceSpeedScale = 1.0f;

	//목표와의 각도차이
	float MinTrackAngle = KINDA_SMALL;
	float MaxTrackAngle = 15.0f;

	// 최대  속도 추적 각도(degree)
	float MaxTrackAngleSpeed = MaxTrackAngle*0.6f;
#pragma endregion
};
