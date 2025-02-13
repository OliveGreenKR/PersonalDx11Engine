#include "GameObject.h"
#include "Model.h"
#include "RigidBodyComponent.h"


UGameObject::UGameObject()
{
	RootActorComp = make_shared<UActorComponent>();
	RigidBody = make_shared<URigidBodyComponent>();
}

UGameObject::UGameObject(const shared_ptr<UModel>& InModel) : Model(InModel)
{
	RigidBody = make_shared<URigidBodyComponent>();
	RootActorComp = make_shared<UActorComponent>();
}

void UGameObject::PostInitialized()
{
	auto CompPtr = RootActorComp.get();
	CompPtr->SetOwner(this);
	//for temp
	RootActorComp.get()->AddChild(RigidBody);
}

void UGameObject::PostInitializedComponents()
{
	auto CompPtr = RootActorComp.get();
	if (CompPtr)
	{
		CompPtr->BroadcastPostInitializedForComponents();
	}
}

void UGameObject::Tick(const float DeltaTime)
{
	UpdateComponents(DeltaTime);
	UpdateMovement(DeltaTime);
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
	//TODO:: ComponentsInterface  + vector
	//find all Tickable compo and call Tick
	auto CompPtr = RootActorComp.get();
	if (CompPtr)
	{
		CompPtr->BroadcastTick(DeltaTime);
	}
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

bool UGameObject::IsPhysicsSimulated() const
{
	return RigidBody.get()->bIsSimulatedPhysics;
}

bool UGameObject::IsGravity() const
{
	return RigidBody.get()->bGravity;
}

void UGameObject::SetGravity(const bool InBool)
{
	RigidBody.get()->bGravity = InBool;
}

void UGameObject::SetPhysics(const bool InBool)
{
	RigidBody.get()->bIsSimulatedPhysics = InBool;
}

void UGameObject::SetFrictionKinetic(const float InValue)
{
	RigidBody.get()->SetFrictionKinetic(InValue);
}

void UGameObject::SetFrictionStatic(const float InValue)
{
	RigidBody.get()->SetFrictionStatic(InValue);
}

Vector3 UGameObject::GetCurrentVelocity() const
{
	return RigidBody.get()->GetVelocity();
}

Vector3 UGameObject::GetCurrentAngularVelocity() const
{
	return RigidBody.get()->GetAngularVelocity();
}

void UGameObject::InitializePhysics()
{
	auto SelfPtr = shared_from_this();
	if (auto RigidPtr = RigidBody.get())
	{
		RigidPtr->bIsSimulatedPhysics = true;
		RigidPtr->SetMaxSpeed(MaxSpeed);
		RigidPtr->bGravity = false;
	}
}
