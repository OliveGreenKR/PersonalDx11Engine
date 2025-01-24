#include "Camera.h"
#include "Math.h"

UCamera::UCamera()
{
	UpdateProjectionMatrix();
}

UCamera::UCamera(float fov, float aspectRatio, float nearZ, float farZ)
	: Fov(fov), AspectRatio(aspectRatio), NearZ(nearZ), FarZ(farZ)
{
	UpdateProjectionMatrix();
}

void UCamera::Tick(float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
	UpdateToLookAtObject(DeltaTime);
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

void UCamera::SetLookAtObject(shared_ptr<UGameObject>& InTarget)
{
	if (!InTarget)
		return;
	LookAtObject = InTarget;
}

void UCamera::LookTo(const Vector3& TargetPosition)
{
	Vector3 Direction = TargetPosition - GetTransform()->Position;
	Direction.Normalize();
	Vector3 CurrentForward = GetForwardVector();
	Quaternion toRotate =  Math::GetRotationBetweenVectors(CurrentForward, Direction);
	AddRotationQuaternion(toRotate);
}

void UCamera::UpdateToLookAtObject(float DeltaTime)
{
	auto TargetObject = LookAtObject.lock();
	if (!bLookAtObject || !TargetObject)
		return;

	XMVECTOR CurrentPos = XMLoadFloat3(&GetTransform()->Position);
	XMVECTOR TargetPos = XMLoadFloat3(&TargetObject->GetTransform()->Position);

	Vector3 CurrentF = GetForwardVector();
	// 현재 전방 벡터를 XMVECTOR로 로드
	XMVECTOR CurrentForward = XMLoadFloat3(&CurrentF);

	// 목표 방향 계산 및 정규화
	XMVECTOR ToTarget = XMVector3Normalize(XMVectorSubtract(TargetPos, CurrentPos));

	// 현재 방향과 목표 방향 사이의 회전 계산
	XMVECTOR TargetRotation = Math::GetRotationBetweenVectors(CurrentForward, ToTarget);

	// 현재 회전을 XMVECTOR로 로드
	XMVECTOR CurrentRotation = XMLoadFloat4(&GetTransform()->Rotation);

	// 각도 차이 계산 (회전 Quaternion의 각도를 추출)
	float DiffAngleRad = 2.0f * XMScalarACos(XMVectorGetW(TargetRotation));
	float DiffAngleDegrees = XMConvertToDegrees(DiffAngleRad);

	if (DiffAngleDegrees > KINDA_SMALL)
	{
		// 회전 속도 계산 (MaxSpeedAngle 기준으로 정규화)
		float SpeedFactor = Math::Clamp(DiffAngleDegrees / MaxTrackSpeedAngle, 0.0f,1.0f);
		float RotationAmount = min(
			MaxRotationSpeed * SpeedFactor * DeltaTime / DiffAngleDegrees,
			1.0f
		);

		XMVECTOR ResultRotation;
		if (DiffAngleDegrees > MaxDiffAngle)
		{
			// MaxDiffAngle까지만 회전
			float LimitFactor = Math::Clamp(MaxDiffAngle / DiffAngleDegrees, 0.0f, 1.0f);
			// 제한된 회전으로 Slerp
			ResultRotation = Math::Slerp(CurrentRotation,
								   XMQuaternionMultiply(CurrentRotation, TargetRotation),
								   LimitFactor);
		}
		else
		{
			// 일반적인 회전 보간
			ResultRotation = Math::Slerp(CurrentRotation,
								   XMQuaternionMultiply(CurrentRotation, TargetRotation),
								   RotationAmount);
		}

		// 결과 회전을 Quaternion으로 변환하여 적용
		Quaternion NewRotation;
		XMStoreFloat4(&NewRotation, ResultRotation);
		SetRoatationQuaternion(NewRotation);
	}
}


void UCamera::OnTransformChanged()
{
	bIsViewDirty = true;
}

void UCamera::UpdatDirtyView() 
{
	UpdateViewMatrix();
	UpdateFrustum();
}

void UCamera::UpdateProjectionMatrix()
{
	ProjectionMatrix = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ); 
}

void UCamera::UpdateViewMatrix()
{
	//베이스 상태
	XMVECTOR vLookAt = XMLoadFloat3(&Vector3::Forward);
	XMVECTOR vUp = XMLoadFloat3(&Vector3::Up);

	//현재 상태
	XMVECTOR vRotation = XMLoadFloat4(&GetTransform()->Rotation);
	XMVECTOR vPosition = XMLoadFloat3(&GetTransform()->Position);

	XMVECTOR currentLookAt = XMVector3Rotate(vLookAt, vRotation);
	currentLookAt = XMVectorAdd(vPosition, currentLookAt);
	const XMVECTOR currentUp = XMVector3Rotate(vUp, vRotation);
	
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


