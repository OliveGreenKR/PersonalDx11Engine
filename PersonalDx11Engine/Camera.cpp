#include "Camera.h"
#include "Math.h"

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

void UCamera::PostInitialized()
{
	UGameObject::PostInitialized();
	Transform.OnTransformChangedDelegate.Bind(shared_from_this(), &UCamera::OnTransformChanged, "OnTransformChanged");
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

	// ��� ��鿡 ���� �� �˻�
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
	Vector3 Direction = TargetPosition - GetTransform()->GetPosition();
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
	//��ǥ ����
	XMVECTOR CurrentPos = XMLoadFloat3(&GetTransform()->GetPosition());
	XMVECTOR TargetPos = XMLoadFloat3(&TargetObject->GetTransform()->GetPosition());
	XMVECTOR CurrentRotation = XMLoadFloat4(&GetTransform()->GetQuaternionRotation());

	//��ǥ�� ����
	XMVECTOR DesiredForward = XMVectorSubtract(TargetPos, CurrentPos);
	DesiredForward = XMVector3Normalize(DesiredForward);

	XMVECTOR WorldUp = XMVector::XMUp();

	XMVECTOR Right = XMVector3Cross(WorldUp, DesiredForward);
	Right = XMVector3Normalize(Right);

	XMVECTOR DesiredUp = XMVector3Cross(DesiredForward, Right);
	DesiredUp = XMVector3Normalize(DesiredUp);

	//ȸ�����
	XMMATRIX RotationMatrix = XMMatrixIdentity();
	RotationMatrix.r[0] = Right;
	RotationMatrix.r[1] = DesiredUp;
	RotationMatrix.r[2] = DesiredForward;

	XMVECTOR TargetRotation = XMQuaternionRotationMatrix(RotationMatrix);

	// ���� ȸ���� ��ǥ ȸ�� ������ ���� ���� ���
	float DiffAngleRad = XMScalarACos(
		XMVectorGetX(XMQuaternionDot(CurrentRotation, TargetRotation))
	);
	float DiffAngleDegrees = XMConvertToDegrees(DiffAngleRad);

	// ���� ����
	if (DiffAngleDegrees > KINDA_SMALL)
	{
		float SpeedFactor = Math::Clamp(DiffAngleDegrees / MaxTrackSpeedAngle, 0.0f, 1.0f);
		float RotationAmount = min(
			MaxRotationSpeed * SpeedFactor * DeltaTime / DiffAngleDegrees,
			1.0f
		);

		XMVECTOR ResultRotation;
		if (DiffAngleDegrees > MaxDiffAngle)
		{
			float LimitFactor = Math::Clamp(MaxDiffAngle / DiffAngleDegrees, 0.0f, 1.0f);
			ResultRotation = XMQuaternionSlerp(
				CurrentRotation,
				TargetRotation,
				LimitFactor * RotationAmount
			);
		}
		else
		{
			ResultRotation = XMQuaternionSlerp(
				CurrentRotation,
				TargetRotation,
				RotationAmount
			);
		}

		Quaternion NewRotation;
		XMStoreFloat4(&NewRotation, ResultRotation);
		SetRotationQuaternion(NewRotation);
	}
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
	//���̽� ����
	XMVECTOR vLookAt = XMVector::XMForward();
	XMVECTOR vUp = XMVector::XMUp();

	//���� ����
	XMVECTOR vRotation = XMLoadFloat4(&GetTransform()->GetQuaternionRotation());
	XMVECTOR vPosition = XMLoadFloat3(&GetTransform()->GetPosition());

	XMVECTOR currentLookAt = XMVector3Rotate(vLookAt, vRotation);
	currentLookAt = XMVectorAdd(vPosition, currentLookAt);

	XMVECTOR currentUp = XMVector3Rotate(vUp, vRotation);

	// �� ��� ���
	ViewMatrix = XMMatrixLookAtLH(vPosition, currentLookAt, currentUp);
}

void UCamera::UpdateFrustum() const
{
	Matrix ViewProj = XMMatrixMultiply(ViewMatrix, ProjectionMatrix);
	CalculateFrustum(ViewProj);
}

void UCamera::CalculateFrustum(Matrix& InViewProj) const 
{
	// ��Ŀ��� �������� ��� ����
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


