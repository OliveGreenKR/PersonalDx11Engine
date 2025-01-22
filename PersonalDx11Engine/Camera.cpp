#include "Camera.h"

UCamera::UCamera()
{
	UpdateProjectionMatrix();
}

Matrix UCamera::GetViewMatrix() const
{
	XMVECTOR up, position, lookat;
	Vector3 Up = V3::Up();
	Vector3 Forward = V3::Forward();
	up = XMLoadFloat3(&Up);
	lookat = XMLoadFloat3(&Forward);

	position = XMLoadFloat3(&GetTransform().Position);

	const Vector3 Rotation = GetTransform().Rotation;
	Matrix RotateMatrix = XMMatrixRotationRollPitchYaw(Rotation.x,Rotation.y,Rotation.z);

	lookat = XMVector3TransformCoord(lookat, RotateMatrix);
	up = XMVector3TransformCoord(up, RotateMatrix);

	lookat = XMVectorAdd(position, lookat);

	return XMMatrixLookAtLH(position, lookat, up);
}

void UCamera::SetProjectionParameters(float InFov, float InAspectRatio, float InNearZ, float InFarZ)
{
	Fov = InFov;
	AspectRatio = InAspectRatio;
	NearZ = InNearZ;
	FarZ = InFarZ;
	ProjectionMatrix = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ);
}

//ViewMatrix = XMMatrixInverse(nullptr, GetWorldMatrix());