#include "GameObject.h"
#include "Model.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"


UGameObject::UGameObject()
{
	RootActorComp = UActorComponent::Create<UActorComponent>();
	auto CompPtr = RootActorComp.get();
	CompPtr->SetOwner(this);
}

UGameObject::UGameObject(const shared_ptr<UModel>& InModel) : Model(InModel)
{
	RootActorComp = UActorComponent::Create<UActorComponent>();
	auto CompPtr = RootActorComp.get();
	CompPtr->SetOwner(this);
}

void UGameObject::PostInitialized()
{
}

void UGameObject::PostInitializedComponents()
{
	auto CompPtr = RootActorComp.get();
	//Components Initialze
	if (CompPtr)
	{
		CompPtr->BroadcastPostitialized();
	}

	if( auto CollisionComp = RootActorComp.get()->FindChildByType<UCollisionComponent>())
	{
		CollisionComp->OnCollisionEnter.Bind(shared_from_this(), &UGameObject::OnCollisionBegin, "OnCollisionBegin_GameObject");
	}
}

void UGameObject::Tick(const float DeltaTime)
{
	UpdateMovement(DeltaTime);
	UpdateComponents(DeltaTime);
}

void UGameObject::SetPosition(const Vector3& InPosition)
{
	Transform.SetPosition(InPosition);
}

void UGameObject::SetRotationEuler(const Vector3& InEulerAngles)
{
	Transform.SetEulerRotation(InEulerAngles);
}

void UGameObject::SetRotationQuaternion(const Quaternion& InQuaternion)
{
	Transform.SetRotation(InQuaternion);
}

void UGameObject::SetScale(const Vector3& InScale)
{
	Transform.SetScale(InScale);
}

void UGameObject::AddPosition(const Vector3& InDelta)
{

	Transform.AddPosition(InDelta);
}

void UGameObject::AddRotationEuler(const Vector3& InEulerDelta)
{
	Transform.AddEulerRotation(InEulerDelta);
}

void UGameObject::AddRotationQuaternion(const Quaternion& InQuaternionDelta)
{
	Transform.AddRotation(InQuaternionDelta);
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

void UGameObject::AddActorComponent(shared_ptr<UActorComponent>& InActorComp)
{
	RootActorComp->AddChild(InActorComp);
}

void UGameObject::UpdateComponents(const float DeltaTime)
{
	//find all Tickable compo and call Tick
	auto CompPtr = RootActorComp.get();
	if (CompPtr)
	{
		CompPtr->BroadcastTick(DeltaTime);
	}
}

void UGameObject::OnCollisionBegin(const FCollisionEventData& InCollision)
{
	if (!InCollision.CollisionResult.bCollided)
		return;

	DebugColor = Color::Green();
}

void UGameObject::OnCollisionEnd(const FCollisionEventData& InCollision)
{
	if (InCollision.CollisionResult.bCollided)
		return;

	DebugColor = Color::White();
}
//좌표기반 움직임만
void UGameObject::StartMove(const Vector3& InDirection)
{
	if (InDirection.LengthSquared() < KINDA_SMALL)
		return;
	bIsMoving = true;
	TargetPosition = Transform.GetPosition() + InDirection.GetNormalized() * MaxSpeed;
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

	Vector3 Current = Transform.GetPosition();
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

	if (auto RigidComp = RootActorComp.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->ApplyForce(InForce);
	}
}

Vector3 UGameObject::GetCurrentVelocity() const
{
	if (!IsPhysicsSimulated())
		return Vector3::Zero;

	if (auto RigidComp = RootActorComp.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->GetVelocity();
	}

	return Vector3::Zero;
}

bool UGameObject::IsGravity() const
{
	if (!IsPhysicsSimulated())
		return false;
	if (auto RigidComp = RootActorComp.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->bGravity;
	}
	return false;
}

bool UGameObject::IsPhysicsSimulated() const
{
	if (!RootActorComp.get())
		return false;
	if (auto RigidComp = RootActorComp.get()->FindChildByType<URigidBodyComponent>())
	{
		return RigidComp->bIsSimulatedPhysics;
	}
	return false;
}

void UGameObject::SetGravity(const bool InBool)
{
	if (!IsPhysicsSimulated())
		return;
	if (auto RigidComp = RootActorComp.get()->FindChildByType<URigidBodyComponent>())
	{
		RigidComp->bGravity = InBool;
	}
}

void UGameObject::SetPhysics(const bool InBool)
{
	if (!IsPhysicsSimulated())
		return;
	if (auto RigidComp = RootActorComp.get()->FindChildByType<URigidBodyComponent>())
	{
		RigidComp->bIsSimulatedPhysics = InBool;
	}
}

void UGameObject::AddActorComponent(const shared_ptr<UActorComponent>& InActorComp)
{
	if (!InActorComp.get() || !RootActorComp.get())
		return;
	RootActorComp.get()->AddChild(InActorComp);
}

