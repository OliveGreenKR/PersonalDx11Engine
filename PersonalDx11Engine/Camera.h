#pragma once
#include "GameObject.h"
#include "Frustum.h"
#include "Delegate.h"

class UCamera : public UGameObject
{
public:
	virtual ~UCamera() override = default;

	static std::shared_ptr<UCamera> Create(float fov, uint32_t viewportWidth, uint32_t viewportHeight, float nearZ, float farZ)
	{
		return UGameObject::Create<UCamera>(fov, viewportWidth, viewportHeight, nearZ, farZ);
	}

protected:
	explicit UCamera(float fov, uint32_t viewportWidth, uint32_t viewportHeight, float nearZ, float farZ);
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
	void SetViewportSize(uint32_t Width, uint32_t Height);

	uint32_t GetViewportWidth() const { return ViewportWidth; }
	uint32_t GetViewportHeight() const { return ViewportHeight; }
	float GetAspectRatio() const { return static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight); }

	bool IsInView(const Vector3& Position);
	void SetLookAtObject(UGameObject* InTarget);

	void LookAt(const Vector3& TargetPosition);
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
	uint32_t ViewportWidth = 1280;  
	uint32_t ViewportHeight = 720;  
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
