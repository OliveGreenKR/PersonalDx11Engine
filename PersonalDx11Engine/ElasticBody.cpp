#include "ElasticBody.h"
#include "ActorComponent.h"
#include "RigidBodyComponent.h"
#include "CollisionComponent.h"
#include "CollisionDefines.h"


UElasticBody::UElasticBody(const EShape & InShape, const Vector4 & InColor)
{

}

void UElasticBody::Tick(const float DeltaTime)
{
	UGameObject::Tick(DeltaTime);
}

void UElasticBody::PostInitialized()
{
	UGameObject::PostInitialized();



	if (auto RootComp = RootActorComp.get())
	{
		//rigid body 추가 및 초기화
		auto RigidBodyComp = UActorComponent::Create<URigidBodyComponent>();
		AddActorComponent(RigidBodyComp);


		//collsion body 추가 및 초기화
		auto ColllsionComp = UActorComponent::Create<UCollisionComponent>(ECollisionShapeType::Sphere, Vector3::Zero);
		ColllsionComp;
	}
}

void UElasticBody::PostInitializedComponents()
{
	UGameObject::PostInitializedComponents();
}

void UElasticBody::OnCollisionBegin(const FCollisionEventData & InCollision)
{
}

void UElasticBody::OnCollisionStay(const FCollisionEventData & InCollision)
{
}

void UElasticBody::OnCollisionEnd(const FCollisionEventData & InCollision)
{
}
