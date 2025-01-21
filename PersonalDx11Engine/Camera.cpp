#include "Camera.h"

Matrix UCamera::GetViewMatrix()
{
	if (bUpdateNeeded)
	{
		ViewMatrix = XMMatrixInverse(nullptr, GetWorldMatrix());
		bUpdateNeeded = false;
	}
	return ViewMatrix;
}

Matrix UCamera::GetProjectionMatrix()
{
	if (bUpdateNeeded)
	{
		ProjectionMatrix =  XMMatrixPerspectiveFovLH(Fov, AspectRatio, NearZ, FarZ);
		bUpdateNeeded = false;
	}

	return ProjectionMatrix;
}

void UCamera::SetProjectionParameters(float InFov, float InAspectRatio, float InNearZ, float InFarZ)
{
	Fov = InFov;
	AspectRatio = InAspectRatio;
	NearZ = InNearZ;
	FarZ = InFarZ;
	bUpdateNeeded = true;

}
