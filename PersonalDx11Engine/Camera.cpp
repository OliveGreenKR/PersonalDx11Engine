#include "Camera.h"
#include "Debug.h"

UCamera::UCamera(float fov, uint32_t viewportWidth, uint32_t viewportHeight, float nearZ, float farZ)
	: Fov(fov), ViewportWidth(viewportWidth), ViewportHeight(viewportHeight), NearZ(nearZ), FarZ(farZ)
{
	MaxTrackAngle = 15.0f;

	UpdateProjectionMatrix();
}

void UCamera::Tick(const float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
	if(bLookAtObject)
		UpdateToLookAtObject(DeltaTime);
}

void UCamera::PostInitialized()
{
	UGameObject::PostInitialized();
	GetRootComp()->OnWorldTransformChangedDelegate.Bind(this, &UCamera::OnTransformChanged, "OnTransformChanged_Camera");
}

const Matrix UCamera::GetViewMatrix() const
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

void UCamera::SetViewportSize(uint32_t Width, uint32_t Height)
{
	ViewportWidth = Width;
	ViewportHeight = Height;
	UpdateProjectionMatrix();  // 비율이 바뀌었으므로 투영 행렬 업데이트
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

void UCamera::LookAt(const Vector3& TargetPosition)
{
	GetRootComp()->LookAt(TargetPosition);
}

void UCamera::LookTo()
{
	auto TargetObject = LookAtObject.lock();
	if (TargetObject)
	{
		LookAt(TargetObject->GetWorldTransform().Position);
	}
}

void UCamera::UpdateToLookAtObject(float DeltaTime)
{
	auto TargetObject = LookAtObject.lock();
	if (!bLookAtObject || !TargetObject)
		return;

	// 1. 목표 방향 계산
	Vector3 TargetPosition = TargetObject->GetWorldTransform().Position;
	Vector3 CurrentPosition = GetWorldTransform().Position;
	Vector3 DesiredDirection = TargetPosition - CurrentPosition;

	// 방향 벡터가 너무 작으면 계산 중단
	if (DesiredDirection.LengthSquared() < KINDA_SMALL)
		return;

	DesiredDirection.Normalize();
	Vector3 CurrentForward = GetWorldForward();

	// 2. 현재 방향과 목표 방향 간의 각도 계산
	float DotProduct = Vector3::Dot(CurrentForward, DesiredDirection);
	DotProduct = Math::Clamp(DotProduct, -1.0f, 1.0f);
	float AngleDiff = Math::RadToDegree(std::acos(DotProduct)); // [0:180]

	//// 3. 최소 각도 이하면 회전하지 않음
	//if (AngleDiff < MinTrackAngle)
	//	return;

	// 4. 회전 방향 결정
	Vector3 RotAxis = Vector3::Cross(CurrentForward, DesiredDirection);
	if (RotAxis.LengthSquared() < KINDA_SMALL)
	{
		// 완전히 반대 방향을 바라보는 경우, 임의의 회전축 사용
		RotAxis = Vector3::Cross(CurrentForward, Vector3::Up());
		if (RotAxis.LengthSquared() < KINDA_SMALL)
			RotAxis = Vector3::Cross(CurrentForward, Vector3::Right());
	}
	RotAxis.Normalize();

	// 5. 회전 각도 제한 및 속도 계산
	float RotationAngle = std::min(AngleDiff, MaxTrackAngle); // 최대 회전 각도 제한
	float SpeedFactor = Math::Clamp(AngleDiff / MaxTrackAngleSpeed, 0.1f, 1.0f); // 회전 속도 계수
	float FinalRotationAngle = RotationAngle * SpeedFactor * DeltaTime * MaxRotationSpeed;

	// 6. 일정 크기 이상의 회전 적용
	if (FinalRotationAngle > 0.01f)
	{
		//LOG("Rotation to Object : %.05f", FinalRotationAngle);
		Quaternion DeltaRotation;
		XMVECTOR RotationQuat = XMQuaternionRotationAxis(XMLoadFloat3(&RotAxis), Math::DegreeToRad(FinalRotationAngle));
		XMStoreFloat4(&DeltaRotation, RotationQuat);
		AddRotationQuaternion(DeltaRotation);
	}
}

void UCamera::UpdatDirtyView() const
{
	UpdateViewMatrix();
	UpdateFrustum();
}

void UCamera::OnTransformChanged(const FTransform& Changed)
{
	bIsViewDirty = true;
}

void UCamera::UpdateProjectionMatrix() const
{
	const float AspectRatio = GetAspectRatio();
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

void UCamera::UpdateViewMatrix() const
{
	//베이스 상태
	XMVECTOR vLookAt = XMVector::XMForward();
	XMVECTOR vUp = XMVector::XMUp();

	//현재 상태
	XMVECTOR vRotation = XMLoadFloat4(&GetWorldTransform().Rotation);
	XMVECTOR vPosition = XMLoadFloat3(&GetWorldTransform().Position);

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

