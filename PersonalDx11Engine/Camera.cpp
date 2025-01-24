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

void UCamera::SetLookAtObject(shared_ptr<UGameObject>& InTarget)
{
	if (!InTarget)
		return;
	LookAtObject = InTarget;
}

void UCamera::LookTo(const Vector3& TargetPosition)
{
	// SIMD 연산을 위해 XMVECTOR 사용
	XMVECTOR vPosition = XMLoadFloat3(&GetTransform()->Position);
	XMVECTOR vTarget = XMLoadFloat3(&TargetPosition);
	XMVECTOR vUp = XMLoadFloat3(&Up);

	// 1. 새로운 Look 방향 계산
	XMVECTOR vLook = XMVectorSubtract(vTarget, vPosition);
	vLook = XMVector3Normalize(vLook);

	// 2. 임시 Right 벡터 계산 (외적)
	XMVECTOR vRight = XMVector3Cross(vUp, vLook);
	vRight = XMVector3Normalize(vRight);

	// 3. 새로운 Up 벡터 계산 (Look과 Right의 외적)
	vUp = XMVector3Cross(vLook, vRight);

	// 4. 결과 저장
	XMStoreFloat3(&LookAt, vLook);
	XMStoreFloat3(&Up, vUp);

	// 5. Quaternion 회전 업데이트- Look과 Up으로부터 회전 행렬 생성
	XMMATRIX rotationMatrix = XMMatrixLookToRH(
		vPosition,           // 카메라 위치
		vLook,              // Look 방향
		vUp                 // Up 벡터
	);

	// 행렬을 Quaternion으로 변환
	XMVECTOR qRotation = XMQuaternionRotationMatrix(rotationMatrix);
	Vector4 newRotataion; 
	XMStoreFloat4(&newRotataion, qRotation);
	SetRoatationQuaternion(newRotataion);
}

void UCamera::UpdateToLookAtObject(float DeltaTime)
{
	//auto Target = LookAtObject.lock();

	//if (!bTrackObject || !Target )
	//	return;

	//// SIMD 최적화를 위한 DirectX 구현
	//XMVECTOR vCurrentPos = XMLoadFloat3(&GetTransform()->Position);
	//XMVECTOR vTargetPos = XMLoadFloat3(&Target->GetTransform()->Position);
	//XMVECTOR vCurrentLook = XMLoadFloat3(&LookAt);

	//// 타겟 방향 계산 및 정규화
	//XMVECTOR vTargetDir = XMVectorSubtract(vTargetPos, vCurrentPos);
	//vTargetDir = XMVector3Normalize(vTargetDir);

	//// 두 방향 벡터로부터 회전 쿼터니온 생성
	//XMVECTOR vRotationAxis =XMVector3Cross(vCurrentLook, vTargetDir);
	//// 현재 회전값
	//XMVECTOR vCurrentRotation = XMLoadFloat4(&GetTransform()->Rotation);
	//float dotProduct = DirectX::XMVectorGetX(DirectX::XMVector3Dot(vCurrentLook, vTargetDir));
	//float rotationAngle = std::acos(dotProduct);

	//DirectX::XMVECTOR vRotationQuat = DirectX::XMQuaternionRotationAxis(
	//	vRotationAxis,
	//	rotationAngle
	//);

	//DirectX::XMVECTOR vCurrentRotation = DirectX::XMLoadFloat4(&GetTransform()->Rotation);

	//float SlerpFactor = std::min(2.0f * DeltaTime, 1.0f);
	//DirectX::XMVECTOR vFinalRotation = DirectX::XMQuaternionSlerp(
	//	vCurrentRotation,
	//	DirectX::XMQuaternionMultiply(vCurrentRotation, vRotationQuat),
	//	SlerpFactor);

	//DirectX::XMFLOAT4 finalRotation;
	//DirectX::XMStoreFloat4(&finalRotation, vFinalRotation);
	//SetRotationQuaternion(finalRotation);
	
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


