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
	XMVECTOR Forward = XMLoadFloat3(&CurrentF);
	XMVECTOR Up = XMVector::XMUp(); //world UP
	XMVECTOR Right = XMVector3Cross(Forward, Up);

	XMVECTOR ToTarget = XMVector3Normalize(XMVectorSubtract(TargetPos, CurrentPos));

	//Yaw 각도 계산
	XMVECTOR ProjectedTarget = XMVector3Normalize(XMVectorSetY(ToTarget, 0.0f));
	XMVECTOR ProjectedForward = XMVector3Normalize(XMVectorSetY(Forward, 0.0f));

	// 각도 차이 계산 
	float YawAngle = atan2(XMVectorGetX(ToTarget), XMVectorGetZ(ToTarget));
	float PitchAngle = asin(XMVectorGetY(ToTarget));

	//회전적용 yaw->pitch
	XMVECTOR YawRotation = XMQuaternionRotationAxis(Up, YawAngle);
	XMVECTOR PitchRotation = XMQuaternionRotationAxis(Right, PitchAngle);

	XMVECTOR CurrentRotation = XMLoadFloat4(&GetTransform()->Rotation);
	XMVECTOR TargetRotation = XMQuaternionMultiply(YawRotation, PitchRotation);

	float DiffAngleRad = XMScalarACos(
		XMVectorGetX(XMQuaternionDot(CurrentRotation, TargetRotation))
	);
	float DiffAngleDegrees = XMConvertToDegrees(DiffAngleRad);
	// 각도 차이가 미미한 경우 회전하지 않음
	if (DiffAngleDegrees > KINDA_SMALL)
	{
		// 회전 속도 계산 (MaxTrackSpeedAngle 기준으로 정규화)
		float SpeedFactor = Math::Clamp(DiffAngleDegrees / MaxTrackSpeedAngle, 0.0f, 1.0f);

		// 이번 프레임에서 회전할 비율 계산
		float RotationAmount = min(
			MaxRotationSpeed * SpeedFactor * DeltaTime / DiffAngleDegrees,
			1.0f
		);

		XMVECTOR ResultRotation;
		if (DiffAngleDegrees > MaxDiffAngle)
		{
			// 한 프레임에서 회전할 수 있는 최대 각도를 제한
			float LimitFactor = Math::Clamp(MaxDiffAngle / DiffAngleDegrees, 0.0f, 1.0f);
			ResultRotation = XMQuaternionSlerp(
				CurrentRotation,
				TargetRotation,
				LimitFactor * RotationAmount
			);
		}
		else
		{
			// 일반적인 회전 보간
			ResultRotation = XMQuaternionSlerp(
				CurrentRotation,
				TargetRotation,
				RotationAmount
			);
		}

		// 결과 회전을 적용
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
	XMVECTOR vLookAt = XMVector::XMForward();
	XMVECTOR vUp = XMVector::XMUp();

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


