#include "ElasticBody.h"
#include "ActorComponent.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "Model.h"

#pragma region Getter Setter
const Vector3& UElasticBody::GetVelocity() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetVelocity();
	}
	static Vector3 DefaultVelocity = Vector3::Zero;
	return DefaultVelocity;
}

const Vector3& UElasticBody::GetAngularVelocity() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetAngularVelocity();
	}
	static Vector3 DefaultAngularVelocity = Vector3::Zero;
	return DefaultAngularVelocity;
}

float UElasticBody::GetSpeed() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetSpeed();
	}
	return 0.0f;
}

float UElasticBody::GetMass() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetMass();
	}
	return 0.0f;
}

Vector3 UElasticBody::GetRotationalInertia() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetRotationalInertia();
	}
	return Vector3::Zero;
}

float UElasticBody::GetRestitution() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetRestitution();
	}
	return 0.0f;
}

float UElasticBody::GetFrictionKinetic() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetFrictionKinetic();
	}
	return 0.0f;
}

float UElasticBody::GetFrictionStatic() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetFrictionStatic();
	}
	return 0.0f;
}

void UElasticBody::SetMass(float InMass) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetMass(InMass);
	}
}

void UElasticBody::SetMaxSpeed(float InSpeed) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetMaxSpeed(InSpeed);
	}
}

void UElasticBody::SetMaxAngularSpeed(float InSpeed) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetMaxAngularSpeed(InSpeed);
	}
}

void UElasticBody::SetGravityScale(float InScale) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetGravityScale(InScale);
	}
}

void UElasticBody::SetFrictionKinetic(float InFriction) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetFrictionKinetic(InFriction);
	}
}

void UElasticBody::SetFrictionStatic(float InFriction) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetFrictionStatic(InFriction);
	}
}

void UElasticBody::SetRestitution(float InRestitution) {
	if (auto RigidPtr = Rigid.lock()) {
		RigidPtr->SetRestitution(InRestitution);
	}
}

void UElasticBody::SetShape(EShape InShape)
{
	if (auto CollisionPtr = Collision.lock())
	{
		CollisionPtr->SetShape(GetCollisionShape(InShape));
	}

	//TODO : 형태에 따른 모델 설정
	switch (InShape)
	{
		case EShape::Box : 
		{
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

UElasticBody::UElasticBody() {}

void UElasticBody::Tick(const float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
}

void UElasticBody::PostInitialized()
{
	UGameObject::PostInitialized();

	//attach actor Comp
	if (auto RootComp = RootActorComp.get())
	{
		//rigid body 추가 및 초기화
		auto RigidBodyComp = UActorComponent::Create<URigidBodyComponent>();
		if (RigidBodyComp.get())
		{
			Rigid = RigidBodyComp;
			AddActorComponent(RigidBodyComp);
		}

		//collsion body 추가 및 초기화
		auto CollisionComp = UActorComponent::Create<UCollisionComponent>(GetCollisionShape(Shape), Vector3::Zero);
		if (CollisionComp.get())
		{
			Collision = CollisionComp;
			CollisionComp->BindRigidBody(RigidBodyComp);
			CollisionComp->SetHalfExtent(GetTransform()->GetScale() * 0.5f);
		}
	}
}

void UElasticBody::PostInitializedComponents()
{
	UGameObject::PostInitializedComponents();
}

void UElasticBody::SetActive(bool bActive)
{
	bIsActive = bActive;
	RootActorComp->SetActive(bIsActive);
}

void UElasticBody::Reset()
{
	// 컴포넌트 상태 초기화
	if (auto rigid = Rigid.lock())
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