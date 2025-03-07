#include "Camera.h"

UCamera::UCamera(float fov, float aspectRatio, float nearZ, float farZ)
	: Fov(fov), AspectRatio(aspectRatio), NearZ(nearZ), FarZ(farZ)
{
	UpdateProjectionMatrix();
}

void UCamera::Tick(float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
	if(bLookAtObject)
		UpdateToLookAtObject(DeltaTime);
}

void UCamera::PostInitialized()
{
	UGameObject::PostInitialized();
	Transform.OnTransformChangedDelegate.Bind(shared_from_this(), &UCamera::OnTransformChanged, "OnTransformChanged_Camera");
}

const Matrix UCamera::GetViewMatrix()
{
	//TODO cache dirty check 
	if (bIsViewDirty)
	{
		UpdatDirtyView();
		bIsViewDirty = false;
	}
	return ViewMatrix;
}

const Matrix UCamera::GetProjectionMatrix() const
{
	return ProjectionMatrix;
}

bool UCamera::IsInView(const Vector3& Position)
{
	if (bIsViewDirty)
	{
		UpdatDirtyView();
	}

	// 모든 평면에 대해 점 검사
	if (!ViewFrustum.Near.IsInFront(Position)) return false;
	if (!ViewFrustum.Far.IsInFront(Position)) return false;
	if (!ViewFrustum.Left.IsInFront(Position)) return false;
	if (!ViewFrustum.Right.IsInFront(Position)) return false;
	if (!ViewFrustum.Top.IsInFront(Position)) return false;
	if (!ViewFrustum.Bottom.IsInFront(Position)) return false;

	return true;
}

void UCamera::SetLookAtObject(UGameObject* InTarget)
{
	if (!InTarget)
		return;
	LookAtObject = InTarget->shared_from_this();
}

void UCamera::LookTo(const Vector3& TargetPosition)
{
	Vector3 Direction = TargetPosition - GetTransform()->GetPosition();
	Direction.Normalize();
	Vector3 CurrentForward = GetNormalizedForwardVector();
	Quaternion toRotate =  Math::GetRotationBetweenVectors(CurrentForward, Direction);
	AddRotationQuaternion(toRotate);
}

void UCamera::LookTo()
{
	auto TargetObject = LookAtObject.lock();
	if (TargetObject)
	{
		LookTo(TargetObject->GetTransform()->GetPosition());
	}
}

void UCamera::UpdateToLookAtObject(float DeltaTime)
{
	auto TargetObject = LookAtObject.lock();
	if (!bLookAtObject || !TargetObject)
		return;

	// 1. 목표 방향 계산
	Vector3 TargetPosition = TargetObject->GetTransform()->GetPosition();
	Vector3 CurrentPosition = GetTransform()->GetPosition();
	Vector3 DesiredDirection = TargetPosition - CurrentPosition;
	DesiredDirection.Normalize();
	Vector3 CurrentForward = GetNormalizedForwardVector();

	// 2. 현재 방향에서 목표 방향으로의 회전 계산
	Quaternion ToRotate = Math::GetRotationBetweenVectors(CurrentForward, DesiredDirection);

	// 3. 회전 각도 계산
	float DotProduct = Vector3::Dot(CurrentForward, DesiredDirection);
	DotProduct = Math::Clamp(DotProduct, -1.0f, 1.0f);
	float AngleDiff = Math::RadToDegree(std::acos(DotProduct)); //[0:180]

	// 4. 최소 각도 체크
	if (AngleDiff < MinTrackAngle)
		return;

	// 5. 회전 속도 계산 (각도가 클수록 빠르게)
	float SpeedFactor = Math::Clamp(AngleDiff / MaxTrackAngleSpeed, 0.0f, 1.0f);
	float CurrentRotationSpeed = MaxRotationSpeed * SpeedFactor * DeltaTime;

	// 6. 최종 회전 계산
	Quaternion StepRotation;
	if (AngleDiff > MaxTrackAngle)
	{
		float RotateAmount = MaxTrackAngle / AngleDiff;
		StepRotation = Math::Slerp(Quaternion::Identity, ToRotate, RotateAmount);
	}
	else
	{
		float RotateAmount = CurrentRotationSpeed / AngleDiff;
		StepRotation = Math::Slerp(Quaternion::Identity, ToRotate, RotateAmount);
	}

	// 7. 회전 적용
	AddRotationQuaternion(StepRotation);
}
void UCamera::UpdatDirtyView() 
{
	UpdateViewMatrix();
	UpdateFrustum();
}

void UCamera::OnTransformChanged(const FTransform& Changed)
{
	bIsViewDirty = true;
}

void UCamera::UpdateProjectionMatrix()
{
	if (bIs2D)
	{
		float nearPlaneHeight = 2.0f * NearZ * tanf(Fov * 0.5f);
		float nearPlaneWidth = nearPlaneHeight * AspectRatio;
		ProjectionMatrix = XMMatrixOrthographicLH(nearPlaneWidth, nearPlaneHeight, NearZ, FarZ);
	}
	else
	{
		ProjectionMatrix = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ);
	}
}

void UCamera::UpdateViewMatrix()
{
	//베이스 상태
	XMVECTOR vLookAt = XMVector::XMForward();
	XMVECTOR vUp = XMVector::XMUp();

	//현재 상태
	XMVECTOR vRotation = XMLoadFloat4(&GetTransform()->GetRotation());
	XMVECTOR vPosition = XMLoadFloat3(&GetTransform()->GetPosition());

	XMVECTOR currentLookAt = XMVector3Rotate(vLookAt, vRotation);
	currentLookAt = XMVectorAdd(vPosition, currentLookAt);

	XMVECTOR currentUp = XMVector3Rotate(vUp, vRotation);

	// 뷰 행렬 계산
	ViewMatrix = XMMatrixLookAtLH(vPosition, currentLookAt, currentUp);
}

void UCamera::UpdateFrustum() const
{
	Matrix ViewProj = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);
	CalculateFrustum(ViewProj);
}

void UCamera::CalculateFrustum(Matrix& InViewProj) const 
{
	// 행렬에서 프러스텀 평면 추출
	XMFLOAT4X4 M;
	XMStoreFloat4x4(&M, XMMatrixTranspose(InViewProj));

	// Left plane
	ViewFrustum.Left = Plane(
		M._14 + M._11,
		M._24 + M._21,
		M._34 + M._31,
		M._44 + M._41);

	// Right plane
	ViewFrustum.Right = Plane(
		M._14 - M._11,
		M._24 - M._21,
		M._34 - M._31,
		M._44 - M._41);

	// Bottom plane
	ViewFrustum.Bottom = Plane(
		M._14 + M._12,
		M._24 + M._22,
		M._34 + M._32,
		M._44 + M._42);

	// Top plane
	ViewFrustum.Top = Plane(
		M._14 - M._12,
		M._24 - M._22,
		M._34 - M._32,
		M._44 - M._42);

	// Near plane
	ViewFrustum.Near = Plane(
		M._13,
		M._23,
		M._33,
		M._43);

	// Far plane
	ViewFrustum.Far = Plane(
		M._14 - M._13,
		M._24 - M._23,
		M._34 - M._33,
		M._44 - M._43);

	ViewFrustum.NormalizeAll();
}

