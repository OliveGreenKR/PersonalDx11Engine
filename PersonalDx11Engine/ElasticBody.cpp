#include "ElasticBody.h"
#include "ActorComponent.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"
#include "PrimitiveComponent.h"
#include "ResourceManager.h"
#include "Model.h"
#include "random.h"
#include "TypeCast.h"
#include "Material.h"
#include "define.h"
#include "BoxComponent.h"
#include "SphereComponent.h"

UElasticBody::UElasticBody(EElasticBodyShape Shape) : bIsActive(true)
{
	//Root to 'Rigid' 
	auto RigidPtr = UActorComponent::Create<URigidBodyComponent>();
	if (RigidPtr)
	{
		auto root = Engine::Cast<USceneComponent>(RigidPtr);
		SetRootComponent(root);

		Rigid = RigidPtr;
		Primitive = AddComponent<UPrimitiveComponent>();

		if (Shape == EElasticBodyShape::Box)
		{
			Primitive->SetModel(UResourceManager::Get()->LoadResource<UModel>(MDL_CUBE));
			Collision = UActorComponent::Create<UBoxComponent>();
		}
		else if (Shape == EElasticBodyShape::Sphere)
		{
			Primitive->SetModel(UResourceManager::Get()->LoadResource<UModel>(MDL_SPHERE_Mid));
			Collision = UActorComponent::Create<USphereComponent>();
		}

		if (Collision)
		{
			Rigid.lock()->AddChild(Collision);
		}
	}
}

UElasticBody::~UElasticBody()
{
	Collision = nullptr;
	Primitive = nullptr;
}

void UElasticBody::Tick(const float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
}

void UElasticBody::PostInitialized()
{
	UGameObject::PostInitialized();

	if (Rigid.lock())
	{
		//collsion body 추가 및 초기화
		if (Collision.get())
		{
			Collision->BindRigidBody(Rigid.lock());
		}
	}
}

void UElasticBody::PostInitializedComponents()
{
	UGameObject::PostInitializedComponents();
}


void UElasticBody::Activate()
{
	UGameObject::Activate();
}

void UElasticBody::DeActivate()
{
	UGameObject::DeActivate();
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
	SetPosition(Vector3::Zero);
	SetRotation(Quaternion::Identity);
}

#pragma region Getter Setter
const Vector3 UElasticBody::GetVelocity() const {
	if (auto RigidPtr = Rigid.lock()) {
		return RigidPtr->GetVelocity();
	}
	static Vector3 DefaultVelocity = Vector3::Zero;
	return DefaultVelocity;
}

const Vector3 UElasticBody::GetAngularVelocity() const {
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
#pragma endregion

void UElasticBody::SetColor(const Vector4& InColor)
{
	if (Primitive)
	{
		Primitive->SetColor(InColor);
	}
}

