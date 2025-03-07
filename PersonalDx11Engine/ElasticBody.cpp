#include "ElasticBody.h"
#include "ActorComponent.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "Model.h"
#include "ModelBufferManager.h"


#pragma region Getter Setter
const Vector3& UElasticBody::GetVelocity() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetVelocity();
	}
	static Vector3 DefaultVelocity = Vector3::Zero;
	return DefaultVelocity;
}

const Vector3& UElasticBody::GetAngularVelocity() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetAngularVelocity();
	}
	static Vector3 DefaultAngularVelocity = Vector3::Zero;
	return DefaultAngularVelocity;
}

float UElasticBody::GetSpeed() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetSpeed();
	}
	return 0.0f;
}

float UElasticBody::GetMass() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetMass();
	}
	return 0.0f;
}

Vector3 UElasticBody::GetRotationalInertia() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetRotationalInertia();
	}
	return Vector3::Zero;
}

float UElasticBody::GetRestitution() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetRestitution();
	}
	return 0.0f;
}

float UElasticBody::GetFrictionKinetic() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetFrictionKinetic();
	}
	return 0.0f;
}

float UElasticBody::GetFrictionStatic() const {
	if (auto RigidPtr = Rigid.get()) {
		return RigidPtr->GetFrictionStatic();
	}
	return 0.0f;
}

void UElasticBody::SetMass(float InMass) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetMass(InMass);
	}
}

void UElasticBody::SetMaxSpeed(float InSpeed) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetMaxSpeed(InSpeed);
	}
}

void UElasticBody::SetMaxAngularSpeed(float InSpeed) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetMaxAngularSpeed(InSpeed);
	}
}

void UElasticBody::SetGravityScale(float InScale) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetGravityScale(InScale);
	}
}

void UElasticBody::SetFrictionKinetic(float InFriction) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetFrictionKinetic(InFriction);
	}
}

void UElasticBody::SetFrictionStatic(float InFriction) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetFrictionStatic(InFriction);
	}
}

void UElasticBody::SetRestitution(float InRestitution) {
	if (auto RigidPtr = Rigid.get()) {
		RigidPtr->SetRestitution(InRestitution);
	}
}

void UElasticBody::SetShape(EShape InShape)
{
	if (auto CollisionPtr = Collision.get())
	{
		CollisionPtr->SetShape(GetCollisionShape(InShape));
	}

	//TODO : 형태에 따른 모델 설정
	switch (InShape)
	{
		case EShape::Box : 
		{
			Model = UModelBufferManager::Get()->GetCubeModel();
			break;
		}
		case EShape::Sphere:
		{
			Model = UModelBufferManager::Get()->GetSphereModel();
			break;
		}
	}
}

void UElasticBody::SetShapeSphere()
{
	SetShape(EShape::Sphere);
}

void UElasticBody::SetShapeBox()
{
	SetShape(EShape::Box);
}
#pragma endregion

UElasticBody::UElasticBody()
{
	Rigid = UActorComponent::Create< URigidBodyComponent>();
	Collision = UActorComponent::Create<UCollisionComponent>();
}

void UElasticBody::Tick(const float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
}

void UElasticBody::PostInitialized()
{
	UGameObject::PostInitialized();

	bDebug = true;

	//attach actor Comp
	if (auto RootComp = RootActorComp.get())
	{
		//rigid body 추가 및 초기화
		if (Rigid.get())
		{
			Rigid.get()->bGravity = false;
			Rigid.get()->bSyncWithOwner = true;
			AddActorComponent(Rigid);
		}

		//collsion body 추가 및 초기화
		if (Collision.get())
		{
			Collision->BindRigidBody(Rigid);
			Collision->SetHalfExtent(GetTransform()->GetScale() * 0.5f);
		}
	}
}

void UElasticBody::PostInitializedComponents()
{
	UGameObject::PostInitializedComponents();
}

void UElasticBody::SetActive(const bool bActive)
{
	Collision->SetActive(bActive);
	Rigid->SetActive(bActive);
	RootActorComp->SetActive(bActive);
	bIsActive = bActive;
}

void UElasticBody::Reset()
{
	// 컴포넌트 상태 초기화
	if (auto rigid = Rigid.get())
	{
		// 물리 상태 초기화
		rigid->Reset();
	}

	// 위치 및 회전 초기화 
	GetTransform()->SetPosition(Vector3::Zero);
	GetTransform()->SetRotation(Quaternion::Identity);
}

ECollisionShapeType UElasticBody::GetCollisionShape(const EShape InShape) const
{
	switch (InShape)
	{
		case EShape::Box:
		{
			return ECollisionShapeType::Box;
			break;
		}
		case EShape::Sphere:
		{
			return ECollisionShapeType::Sphere;
			break;
		}
		default:
		{
			return ECollisionShapeType::Sphere;
			break;
		}
	}
}