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
	//TODO: LookAT
	if (bLookAt)
	{
		UpdateLookAt();
	}
}

Matrix UCamera::GetViewMatrix() const
{
	//TODO cache dirty check 
	if (bIsViewDirty)
	{
		UpdatDirtyView();
		bIsViewDirty = false;
	}
	return ViewMatrix;
}

Matrix UCamera::GetProjectionMatrix() const
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

void UCamera::SetLookAt(shared_ptr<UGameObject>& InTarget)
{
	if (!InTarget)
		return;
	LookAtObject = InTarget;
}

void UCamera::OnTransformChanged()
{
	bIsViewDirty = true;
}

void UCamera::UpdatDirtyView() const
{
	UpdateViewMatrix();
	UpdateFrustum();
}

void UCamera::UpdateProjectionMatrix()
{
	ProjectionMatrix = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ); 
}

void UCamera::UpdateViewMatrix() const
{
	XMVECTOR up, position, lookat;

	up = XMLoadFloat3(&Up);
	lookat = XMLoadFloat3(&LookAt);

	position = XMLoadFloat3(&Transform.Position);

	const Vector3 Rotation = Transform.Rotation;
	Matrix RotateMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);

	lookat = XMVector3TransformCoord(lookat, RotateMatrix);
	up = XMVector3TransformCoord(up, RotateMatrix);

	lookat = XMVectorAdd(position, lookat);

	ViewMatrix = XMMatrixLookAtLH(position, lookat, up);
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

void UCamera::UpdateLookAt()
{
	if (!LookAtObject.lock())
		return;
	LookAt = (LookAtObject.lock()->GetTransform().Position - Transform.Position).GetNormalized();
}
