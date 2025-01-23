#include "Camera.h"

UCamera::UCamera()
{
	UpdateProjectionMatrix();
}

UCamera::UCamera(float fov, float aspectRatio, float nearZ, float farZ)
	: Fov(fov), AspectRatio(aspectRatio), NearZ(nearZ), FarZ(farZ)
{
	UpdateProjectionMatrix();
}

Matrix UCamera::GetViewMatrix() const
{
	//TODO cache dirty check 
	UpdateViewMatrix();
	return ViewMatrix;
}

Matrix UCamera::GetProjectionMatrix() const
{
	return ProjectionMatrix;
}


void UCamera::UpdateProjectionMatrix()
{
	ProjectionMatrix = XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ); 
}

void UCamera::UpdateViewMatrix() const 
{
	XMVECTOR up, position, lookat;
	Vector3 Up = Vector3::Up;
	Vector3 Forward = Vector3::Forward;
	up = XMLoadFloat3(&Up);
	lookat = XMLoadFloat3(&Forward);

	position = XMLoadFloat3(&Transform.Position);

	const Vector3 Rotation = Transform.Rotation;
	Matrix RotateMatrix = XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z);

	lookat = XMVector3TransformCoord(lookat, RotateMatrix);
	up = XMVector3TransformCoord(up, RotateMatrix);

	lookat = XMVectorAdd(position, lookat);

	ViewMatrix = XMMatrixLookAtLH(position, lookat, up);
}
