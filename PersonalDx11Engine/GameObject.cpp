﻿#include "GameObject.h"
#include "Model.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"


UGameObject::UGameObject()
{
	RootComponent = UActorComponent::Create<USceneComponent>();
}

void UGameObject::PostInitialized()
{
	auto CompPtr = RootComponent.get();
	CompPtr->SetOwner(this);
}

void UGameObject::PostInitializedComponents()
{
	auto CompPtr = RootComponent.get();

	//Comp Post Tree Initialize
	if (CompPtr)
	{
		CompPtr->BraodcastPostTreeInitialized();
	}

	if( auto CollisionComp = RootComponent.get()->FindChildByType<UCollisionComponent>())
	{
		CollisionComp->OnCollisionEnter.Bind(shared_from_this(), &UGameObject::OnCollisionBegin, "OnCollisionBegin_GameObject");
		CollisionComp->OnCollisionExit.Bind(shared_from_this(), &UGameObject::OnCollisionEnd, "OnCollisionEnd_GameObject");
	}
}

void UGameObject::Tick(const float DeltaTime)
{
	if (!bIsActive)
	{
		return;
	}
	UpdateMovement(DeltaTime);
	UpdateComponents(DeltaTime);
}

void UGameObject::SetPosition(const Vector3& InPosition)
{
	GetTransform()->SetPosition(InPosition);
}

void UGameObject::SetRotationEuler(const Vector3& InEulerAngles)
{
	GetTransform()->SetEulerRotation(InEulerAngles);
}

void UGameObject::SetRotationQuaternion(const Quaternion& InQuaternion)
{
	GetTransform()->SetRotation(InQuaternion);
}

void UGameObject::SetScale(const Vector3& InScale)
{
	GetTransform()->SetScale(InScale);
}

void UGameObject::AddPosition(const Vector3& InDelta)
{
	GetTransform()->AddPosition(InDelta);
}

void UGameObject::AddRotationEuler(const Vector3& InEulerDelta)
{
	GetTransform()->AddEulerRotation(InEulerDelta);
}

void UGameObject::AddRotationQuaternion(const Quaternion& InQuaternionDelta)
{
	GetTransform()->AddRotation(InQuaternionDelta);
}

const Vector3 UGameObject::GetNormalizedForwardVector() const
{
	Matrix RotationMatrix = GetTransform()->GetRotationMatrix();
	XMVECTOR BaseForward = XMVector::XMForward();
	XMVECTOR TransformedForward = XMVector3Transform(BaseForward, RotationMatrix);
	TransformedForward = XMVector3Normalize(TransformedForward);

	Vector3 Result;
	XMStoreFloat3(&Result, TransformedForward);
	return Result;
}

void UGameObject::UpdateComponents(const float DeltaTime)
{
	//find all Tickable compo and call Tick
	auto CompPtr = RootComponent.get();
	if (CompPtr)
	{
		CompPtr->BroadcastTick(DeltaTime);
	}
}

void UGameObject::Activate()
{
	RootComponent->SetActive(true);
	bIsActive = true;
}

void UGameObject::DeActivate()
{
	RootComponent->SetActive(false);
	bIsActive = false;
}

void UGameObject::OnCollisionBegin(const FCollisionEventData& InCollision)
{
	if (!InCollision.CollisionDetectResult.bCollided)
		return;
}

void UGameObject::OnCollisionEnd(const FCollisionEventData& InCollision)
{
	if (InCollision.CollisionDetectResult.bCollided)
		return;
}
//좌표기반 움직임만
void UGameObject::StartMove(const Vector3& InDirection)
{
	if (InDirection.LengthSquared() < KINDA_SMALL)
		return;
	bIsMoving = true;
	TargetPosition = GetTransform()->GetPosition() + InDirection.GetNormalized() * MaxSpeed;
}

void UGameObject::StopMove()
{
	StopMoveImmediately();
}


void UGameObject::StopMoveImmediately()
{
	bIsMoving = false;
}

void UGameObject::UpdateMovement(const float DeltaTime)
{
	if (bIsMoving == false)
		return;

	Vector3 Current = GetTransform()->GetPosition();
	Vector3 Delta = TargetPosition - Current;

	//정지
	float RemainingDistance = Delta.Length();
	if (RemainingDistance < KINDA_SMALL)
	{
		StopMoveImmediately();
		return;
	}
	// 이번 프레임에서의 이동 거리 계산 (등속 운동)
	Vector3 NewPosition;

	float MoveDistance = MaxSpeed * DeltaTime;
	if (MoveDistance > RemainingDistance)
	{
		NewPosition = TargetPosition;
	}
	else
	{
		NewPosition = Current + Delta.GetNormalized() * MoveDistance;
	}
	 
	SetPosition(NewPosition);
	
}

void UGameObject::ApplyForce(const Vector3&& InForce)
{
	if (!IsPhysicsSimulated())
		return;

	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->ApplyForce(InForce);
	}
}

void UGameObject::ApplyImpulse(const Vector3&& InImpulse)
{
	if (!IsPhysicsSimulated())
		return;

	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->ApplyImpulse(InImpulse);
	}
}

Vector3 UGameObject::GetCurrentVelocity() const
{
	if (!IsPhysicsSimulated())
		return Vector3::Zero;

	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->GetVelocity();
	}

	return Vector3::Zero;
}

float UGameObject::GetMass() const
{
	if (!IsPhysicsSimulated())
		return 0.0f;

	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->GetMass();
	}

	return 0.0f;
}

bool UGameObject::IsGravity() const
{
	if (!IsPhysicsSimulated())
		return false;
	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->bGravity;
	}
	return false;
}

bool UGameObject::IsPhysicsSimulated() const
{
	if (!RootComponent.get())
		return false;
	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->IsActive();
	}
	return false;
}

void UGameObject::SetGravity(const bool InBool)
{
	if (!IsPhysicsSimulated())
		return;
	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		RigidComp->bGravity = InBool;
	}
}

void UGameObject::SetPhysics(const bool InBool)
{
	if (!RootComponent.get())
		return;

	if (auto RigidComp = RootComponent.get()->FindChildByType<URigidBodyComponent>())
	{
		RigidComp->SetActive(InBool);
	}
}
