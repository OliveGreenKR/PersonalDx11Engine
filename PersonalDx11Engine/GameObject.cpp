#include "GameObject.h"
#include "Model.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"


UGameObject::UGameObject()
{
	auto Scene = UActorComponent::Create<USceneComponent>();
	SetRootComponent(Scene);
}

UGameObject::~UGameObject()
{
	RootComponent = nullptr;
}

void UGameObject::PostInitialized()
{ 
	auto CompPtr = RootComponent.get();

	//Comp Post Initialize
	if (CompPtr)
	{
		CompPtr->BroadcastPostInitialized();
	}
}

void UGameObject::PostInitializedComponents()
{
	auto CompPtr = RootComponent.get();

	//Comp Post Tree Initialize
	if (CompPtr)
	{
		CompPtr->BroadcastPostTreeInitialized();
	}

	if( auto CollisionComp = RootComponent.get()->FindChildByType<UCollisionComponentBase>().lock())
	{
		CollisionComp->OnCollisionEnter.Bind(this, &UGameObject::OnCollisionBegin, "OnCollisionBegin_GameObject");
		CollisionComp->OnCollisionExit.Bind(this, &UGameObject::OnCollisionEnd, "OnCollisionEnd_GameObject");
	}

	SetActive(true); //최초 자동 활성화
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
	RootComponent->SetWorldPosition(InPosition);
}

void UGameObject::SetRotationEuler(const Vector3& InEulerAngles)
{
	RootComponent->SetWorldRotationEuler(InEulerAngles);
}

void UGameObject::SetRotation(const Quaternion& InQuaternion)
{
	RootComponent->SetWorldRotation(InQuaternion);
}

void UGameObject::SetScale(const Vector3& InScale)
{
	RootComponent->SetWorldScale(InScale);
}

void UGameObject::AddPosition(const Vector3& InDelta)
{
	RootComponent->AddWorldPosition(InDelta);
}

void UGameObject::AddRotationEuler(const Vector3& InEulerDelta)
{
	RootComponent->AddWorldRotationEuler(InEulerDelta);
}

void UGameObject::AddRotationQuaternion(const Quaternion& InQuaternionDelta)
{
	RootComponent->AddWorldRotation(InQuaternionDelta);
}

const Vector3 UGameObject::GetWorldForward() const
{
	return GetRootComp()->GetWorldForward();
}

const Vector3 UGameObject::GetWorldUp() const
{
	return GetRootComp()->GetWorldUp();
}

const Vector3 UGameObject::GetWorldRight() const
{
	return GetRootComp()->GetWorldRight();
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
	if (bIsActive)
		return;

	RootComponent->SetActive(true);
	bIsActive = true;
}

void UGameObject::DeActivate()
{
	if (!bIsActive)
		return;

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
	TargetPosition = GetWorldTransform().Position + InDirection.GetNormalized() * MovementSpeed;
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

	Vector3 Current = GetWorldTransform().Position;
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

	float MoveDistance = MovementSpeed * DeltaTime;
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

	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		return RigidPtr->ApplyForce(InForce);
	}
}

void UGameObject::ApplyImpulse(const Vector3&& InImpulse)
{
	if (!IsPhysicsSimulated())
		return;

	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		return RigidPtr->ApplyImpulse(InImpulse);
	}
}

Vector3 UGameObject::GetCurrentVelocity() const
{
	if (!IsPhysicsSimulated())
		return Vector3::Zero();

	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		return RigidPtr->GetVelocity();
	}
	return Vector3::Zero();
}

float UGameObject::GetMass() const
{
	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		return RigidPtr->GetMass();
	}

	return 0.0f;
}

bool UGameObject::IsGravity() const
{
	if (!IsPhysicsSimulated())
		return false;
	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		return RigidPtr->IsGravity();
	}
	return false;
}

bool UGameObject::IsPhysicsSimulated() const
{
	if (!RootComponent.get())
		return false;
	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		return RigidPtr->IsActive();
	}
	return false;
}

void UGameObject::SetGravity(const bool InBool)
{
	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		RigidPtr->SetGravity(InBool);
		return;
	}
}

void UGameObject::SetPhysics(const bool InBool)
{
	if (!RootComponent.get())
		return;

	auto RigidComp = RootComponent.get()->FindComponentByType<URigidBodyComponent>();
	if (auto RigidPtr = RigidComp.lock())
	{
		RigidPtr->SetActive(InBool);
		return;
	}
}

void UGameObject::SetRootComponent(std::shared_ptr<USceneComponent>& InSceneComp)
{
	if (!InSceneComp)
		return;

	if (RootComponent)
	{
		RootComponent = nullptr;
	}

	RootComponent = InSceneComp;
	RootComponent->RequestSetOwner(this, UActorComponent::OwnerToken());
	return;
}
