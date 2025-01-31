#include "GameObject.h"
#include "Model.h"
#include "RigidBodyComponent.h"


void UGameObject::Tick(const float DeltaTime)
{
	UpdateComponents(DeltaTime);
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

void UGameObject::SetRotationQuaternion(const Quaternion& InQuaternion)
{
	Transform.SetRotation(InQuaternion);
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

void UGameObject::AddRotationQuaternion(const Quaternion& InQuaternionDelta)
{
	Transform.AddRotation(InQuaternionDelta);
	OnTransformChanged();
}

const Vector3 UGameObject::GetForwardVector() const
{
	Matrix RotationMatrix = GetTransform()->GetRotationMatrix();
	XMVECTOR BaseForward = XMVector::XMForward();
	XMVECTOR TransformedForward = XMVector3Transform(BaseForward, RotationMatrix);
	TransformedForward = XMVector3Normalize(TransformedForward);

	Vector3 Result;
	XMStoreFloat3(&Result, TransformedForward);
	return Result;
}

UModel* UGameObject::GetModel() const
{
	if (auto ptr = Model.lock())
	{
		return ptr.get();
	}
	return nullptr;
}

void UGameObject::UpdateComponents(const float DeltaTime)
{
	//TODO:: ComponentsInterface  + vector
	auto Compo = RigidBody.lock();
	if (Compo)
	{
		Compo->Tick(DeltaTime);
	}
}

void UGameObject::StartMove(const Vector3& InDirection)
{
	if (InDirection.LengthSquared() < KINDA_SMALL)
		return;
	bIsMoving = true;
	if (bIsPhysicsSimulated)
	{
		TargetVelocity = InDirection.GetNormalized() * MaxSpeed;
	}
	else
	{
		TargetPosition = Transform.Position + InDirection.GetNormalized() * MaxSpeed;
	}
}

void UGameObject::StopMove()
{
	if (bIsPhysicsSimulated)
	{
		StopMoveSlowly();
	}
	else
	{
		StopMoveImmediately();
	}
}

void UGameObject::StopMoveSlowly()
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
	if (bIsMoving == false)
		return;

	if(bIsSimul)
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
		CurrentVelocity = RigidBody.lock()->GetVelocity();
	}
	else
	{
		bIsMoving = false;
	}
}

//Update Positin with currentVelocity
void UGameObject::UpdatePosition(const float DeltaTime)
{
	Vector3 NewPosition;
	if (!bIsPhysicsSimulated)
	{
		NewPosition = Vector3::Lerp(Transform.Position, TargetPosition, DeltaTime);
		SetPosition(NewPosition);
	}
	else if (CurrentVelocity.Length() > KINDA_SMALL)
	{
		NewPosition = Transform.Position +
			CurrentVelocity * DeltaTime;
		SetPosition(NewPosition);
	}
	
}

void UGameObject::SetupPyhsics()
{
	auto selfPtr = shared_from_this();
	RigidBody = std::make_shared<URigidBodyComponent>(selfPtr);
	
	if (auto RigidPtr = RigidBody.lock())
	{
		RigidPtr->EnablePhysics(bIsPhysicsSimulated);
		RigidPtr->SetMass(Mass);
		RigidPtr->SetMaxSpeed(MaxSpeed);
		RigidPtr->SetFrictionCoefficient(FrictionCoefficient);
		RigidPtr->EnableGravity(bGravity); //gravity not yet
	}
}

void UGameObject::ApplyForce(const Vector3& Force)
{
	if (auto RigidBodyPtr = RigidBody.lock())
	{
		RigidBodyPtr->ApplyForce(Force);
	}
}

void UGameObject::ApplyImpulse(const Vector3& Impulse)
{
	if (auto RigidBodyPtr = RigidBody.lock())
	{
		RigidBodyPtr->ApplyImpulse(Impulse);
	}
}



