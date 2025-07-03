#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "Debug.h"
#include "PhysicsDefine.h"
#include "PhysicsSystem.h"
#include "TypeCast.h"

#include "PrimitiveComponent.h"

URigidBodyComponent::URigidBodyComponent()
{
   bPhysicsSimulated = true;
}

URigidBodyComponent::~URigidBodyComponent()
{
}

void URigidBodyComponent::PostInitialized()
{
	USceneComponent::PostInitialized();

	//초기 상태 저장
	CachedState.WorldTransform = GetWorldTransform();
}

void URigidBodyComponent::Activate()
{
	USceneComponent::Activate();
	//활성화 명령 전송
}

void URigidBodyComponent::DeActivate()
{
	USceneComponent::DeActivate();
	//비활성화 명령 전송
}


void URigidBodyComponent::Tick(const float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);
	if (!IsActive())
		return;
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	return GetWorldTransform().Position;
}

#pragma region PhysicsObject Life Cycle
void URigidBodyComponent::TickPhysics(const float DeltaTime)
{
	return;
}

void URigidBodyComponent::SetWorldTransform(const FTransform& InWorldTransform)
{
	//기존 트랜스폼 로직 실행
	FTransform OldWorld = GetWorldTransform();

	//잡 전달 여부
	bool bIsUpdate =
		FTransform::IsValidPosition(OldWorld.Position - InWorldTransform.Position) ||
		FTransform::IsValidScale(OldWorld.Scale - InWorldTransform.Scale) ||
		FTransform::IsValidRotation(OldWorld.Rotation, InWorldTransform.Rotation);

	if (!bIsUpdate)
		return;

	//트랜스폼 수정
	USceneComponent::SetWorldTransform(InWorldTransform);
	
	//잡 전달
	bStateDirty = true;
	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobSetWorldTransform>
												 (Engine::Cast<URigidBodyComponent>(shared_from_this()), GetWorldTransform());
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);
}


// 시뮬레이션 플래그
void URigidBodyComponent::SetGravity(const bool InBool) 
{
	bGravity = InBool; 
}

void URigidBodyComponent::RegisterPhysicsSystem()
{
	auto myShared = Engine::Cast<IPhysicsObject>(
		Engine::Cast<URigidBodyComponent>(shared_from_this()));

	assert(myShared, "[Error] InValidPhysicsObejct");
	UPhysicsSystem::Get()->RegisterPhysicsObject(myShared);
}

void URigidBodyComponent::UnRegisterPhysicsSystem()
{
	auto myShared = Engine::Cast<IPhysicsObject>(
		Engine::Cast<URigidBodyComponent>(shared_from_this()));
	assert(myShared, "[Error] InValidPhysicsObejct");
	UPhysicsSystem::Get()->UnregisterPhysicsObject(myShared);
}

// 물리 연산 결과 받아오기(동기화)
void URigidBodyComponent::SynchronizeCachedStateFromSimulated()
{
	CachedState = SimulatedState;
	USceneComponent::SetWorldTransform(CachedState.WorldTransform);
	bStateDirty = false;
}

#pragma endregion



#pragma region PhysicsState Interface
//TODO : Job Enrollment
#pragma endregion

