#include "GameObject.h"
#include "Model.h"


void UGameObject::Tick(const float DeltaTime)
{
	if(bIsMoving)
		UpdateMovement(DeltaTime);
}

void UGameObject::SetPosition(const Vector3& InPosition)
{
	Transform.Position = InPosition;
	OnTransformChanged();
}

void UGameObject::SetRotation(const Vector3& InRotation)
{
	Transform.Rotation = InRotation;
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

void UGameObject::AddRotation(const Vector3& InDelta)
{
	Transform.Rotation += InDelta;
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

void UGameObject::StopMove()
{
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

	if (VelocityDiff.Length() > KINDA_SMALL)
	{
		// 목표 속도를 향해 가속/감속
		CurrentVelocity = CurrentVelocity +
			VelocityDiff.GetNormalized() * CurrentAcceleration * DeltaTime;
	}
	else if (!bIsMoving)
	{
		CurrentVelocity = Vector3::Zero;
		bIsMoving = false;
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



