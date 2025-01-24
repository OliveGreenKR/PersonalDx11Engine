#include "GameObject.h"
#include "Model.h"


void UGameObject::Tick(const float DeltaTime)
{
	UpdateMovement(DeltaTime);
}

void UGameObject::SetPosition(const Vector3& InPosition)
{
	Transform.Position = InPosition;
	OnTransformChanged();
}

void UGameObject::SetRotationEuler(const Vector3& InEulerAngles)
{
	Transform.SetRotation(InEulerAngles);
	OnTransformChanged();
}

void UGameObject::SetScale(const Vector3& InScale)
{
	Transform.Scale = InScale;
	OnTransformChanged();
}

void UGameObject::AddPosition(const Vector3& InDelta)
{
	Transform.Position += InDelta;
	OnTransformChanged();
}

void UGameObject::AddRotationEuler(const Vector3& InEulerDelta)
{
	Transform.AddRotation(InEulerDelta);
	OnTransformChanged();
}

UModel* UGameObject::GetModel() const
{
	if (auto ptr = Model.lock())
	{
		return ptr.get();
	}
	return nullptr;
}

void UGameObject::StartMove(const Vector3& InTarget)
{
	bIsMoving = true;
	TargetVelocity = InTarget.GetNormalized() * MaxSpeed;
	if (!bIsPhysicsBasedMove)
	{
		TargetPosition = InTarget;
	}
}

void UGameObject::StopMoveSlowly()
{
	bIsMoving = false;
	TargetVelocity = Vector3::Zero;
}

void UGameObject::StopMoveImmediately()
{
	bIsMoving = false;
	TargetVelocity = Vector3::Zero;
	CurrentVelocity = TargetVelocity;
}

void UGameObject::UpdateMovement(const float DeltaTime)
{
	UpdateVelocity(DeltaTime);
	UpdatePosition(DeltaTime);
}

//Update Velocity to TargetVelocity
void UGameObject::UpdateVelocity(const float DeltaTime)
{
	const float CurrentAcceleration = bIsMoving ? Acceleration : Deceleration;
	const Vector3 VelocityDiff = TargetVelocity - CurrentVelocity;
	const float DiffSize = VelocityDiff.Length();

	if (VelocityDiff.Length() > KINDA_SMALL)
	{
		// 목표 속도를 향해 가속/감속
		CurrentVelocity = CurrentVelocity +
			VelocityDiff.GetNormalized() * min(DiffSize,CurrentAcceleration * DeltaTime);
	}
}

//Update Positin with currentVelocity
void UGameObject::UpdatePosition(const float DeltaTime)
{
	if (CurrentVelocity.Length() > KINDA_SMALL)
	{
		Vector3 NewPosition = Transform.Position +
			CurrentVelocity * DeltaTime;
		SetPosition(NewPosition);
	}
}



