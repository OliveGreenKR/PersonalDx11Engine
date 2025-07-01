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
	if (!IsActive() || IsStatic())
		return;

    //물리 상태 초기화
	Vector3 Velocity = SimulatedState.Velocity;
	Vector3 AngularVelocity = SimulatedState.AngularVelocity;
	Vector3 AccumulatedForce = SimulatedState.AccumulatedForce;
	Vector3 AccumulatedTorque = SimulatedState.AccumulatedTorque;

	const float InvMass = SimulatedState.InvMass;
	const Vector3 InvRotationalInertia = SimulatedState.InvRotationalInertia;
	const float FrictionKinetic = SimulatedState.FrictionKinetic;
	const float FrictionStatic = SimulatedState.FrictionStatic;
	const float Restitution = SimulatedState.Restitution;

	Vector3 TotalAcceleration = Vector3::Zero();
	Vector3 TotalAngularAcceleration = Vector3::Zero();

	constexpr float DragCoefficient = 0.01f;
	constexpr float RotationalDragCoefficient = 0.1f;
	//공기 저항
	Vector3 DragForce = -Velocity.GetNormalized() * Velocity.LengthSquared() * DragCoefficient;
	Vector3 DragAcceleration = DragForce * InvMass;
	TotalAcceleration += DragAcceleration;
	
	//공기저항 각속도
	Vector3 AngularDragTorque = -AngularVelocity.GetNormalized() *
		AngularVelocity.LengthSquared() *
		RotationalDragCoefficient;
	Vector3 AngularDragAcceleration = Vector3(
		AngularDragTorque.x * InvRotationalInertia.x,
		AngularDragTorque.y * InvRotationalInertia.y,
		AngularDragTorque.z * InvRotationalInertia.z
	);
	TotalAngularAcceleration += AngularDragAcceleration;

	float GravityFactor = GravityScale * ONE_METER;
	// 중력 가속도 추가
	if (bGravity)
	{
		TotalAcceleration += GravityDirection * GravityFactor;
	}

	//마찰력
	Vector3 frictionAccel = Vector3::Zero();
	if (IsValidVelocity(Velocity))
	{

		// 정적 마찰력 영역에서 운동 마찰력 영역으로의 전환 확인
		if (Velocity.Length() < KINDA_SMALL &&
			AccumulatedForce.Length() <= FrictionStatic * GravityFactor / InvMass )
		{
			// 정적 마찰력이 외력을 상쇄
			AccumulatedForce = Vector3::Zero();
			Velocity = Vector3::Zero();
		}
		else
		{
			// 운동 마찰력 계산
			frictionAccel = -Velocity.GetNormalized() * FrictionKinetic * GravityFactor;

			//마찰 제한 적용 -  운동마찰은 객체의 속도를 0까지만 만들 수 있음
			Vector3 NewVelocity = Velocity;
			NewVelocity += frictionAccel * DeltaTime;

			//적용 후에도 속도의 방향이 바뀌지 않음
			if (Vector3::Dot(NewVelocity, Velocity) > KINDA_SMALL)
			{
				TotalAcceleration += frictionAccel;
			}
			else
			{
				Velocity = Vector3::Zero();
			}
		}
	}

	// 각운동 마찰력 처리
	if (IsValidAngularVelocity(AngularVelocity))
	{
		//각 축별로 정적 마찰 검사
		bool bStaticFrictionX = std::abs(AngularVelocity.x) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.x) <= FrictionStatic / InvRotationalInertia.x;
		bool bStaticFrictionY = std::abs(AngularVelocity.y) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.y) <= FrictionStatic / InvRotationalInertia.y;
		bool bStaticFrictionZ = std::abs(AngularVelocity.z) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.z) <= FrictionStatic / InvRotationalInertia.z;

		// 축별로 정적/운동 마찰 적용
		Vector3 frictionAccel;
		frictionAccel.x = bStaticFrictionX ? -AccumulatedTorque.x : -AngularVelocity.x * FrictionKinetic;
		frictionAccel.y = bStaticFrictionY ? -AccumulatedTorque.y : -AngularVelocity.y * FrictionKinetic;
		frictionAccel.z = bStaticFrictionZ ? -AccumulatedTorque.z : -AngularVelocity.z * FrictionKinetic;

		TotalAngularAcceleration += frictionAccel;
	}

	// 외부에서 적용된 힘에 의한 가속도 추가
	if (IsValidForce(AccumulatedForce))
	{
		TotalAcceleration += AccumulatedForce * InvMass;
	}
	if (IsValidTorque(AccumulatedTorque))
	{
		TotalAngularAcceleration += Vector3(
			AccumulatedTorque.x * InvRotationalInertia.x,
			AccumulatedTorque.y * InvRotationalInertia.y,
			AccumulatedTorque.z * InvRotationalInertia.z);
	}

	// 통합된 가속도로 속도 업데이트
	if (IsValidAcceleration(TotalAcceleration))
	{
		Velocity += TotalAcceleration * DeltaTime;
	}

	if (IsValidAngularAcceleration(TotalAngularAcceleration))
	{
		AngularVelocity += TotalAngularAcceleration * DeltaTime;
	}

	// 속도 제한
	ClampVelocities(Velocity, AngularVelocity);

	// 외부 힘 초기화
	AccumulatedForce = Vector3::Zero();
	AccumulatedTorque = Vector3::Zero();

	//상태값 저장
	if (IsValidVelocity(SimulatedState.Velocity - Velocity))
	{
		SimulatedState.Velocity = Velocity;
	}
	if (IsValidAngularVelocity(SimulatedState.AngularVelocity - AngularVelocity))
	{
		SimulatedState.AngularVelocity = AngularVelocity;
	}
	//외부힘은 제로로 초기화
	SimulatedState.AccumulatedForce = AccumulatedForce;
	SimulatedState.AccumulatedTorque = AccumulatedTorque;

	//  저장된 상태값에 따른 위치 업데이트
	P_UpdateTransformByVelocity(DeltaTime);
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

