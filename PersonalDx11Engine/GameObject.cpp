#include "GameObject.h"
#include "Model.h"
#include "RigidBodyComponent.h"


UGameObject::UGameObject()
{
	RigidBody = make_shared<URigidBodyComponent>();
}

UGameObject::UGameObject(const shared_ptr<UModel>& InModel) : Model(InModel)
{
	RigidBody = make_shared<URigidBodyComponent>();
}

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
	//find all Tickable compo and call Tick
	auto RigidPtr = RigidBody.get();
	if (RigidPtr)
	{
		RigidPtr->Tick(DeltaTime);
	}
}

//좌표기반 움직임만
void UGameObject::StartMove(const Vector3& InDirection)
{
	if (InDirection.LengthSquared() < KINDA_SMALL)
		return;
	bIsMoving = true;
	TargetPosition = Transform.Position + InDirection.GetNormalized() * MaxSpeed;
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

	Vector3 Current = GetTransform()->Position;
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
	return RigidBody.get()->GetLinearVelocity();
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
		RigidPtr->SetOwner(SelfPtr);
		RigidPtr->bIsSimulatedPhysics = true;
		RigidPtr->SetMaxSpeed(MaxSpeed);
		RigidPtr->bGravity = false;
	}
}
