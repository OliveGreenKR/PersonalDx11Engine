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

	OnWorldTransformChangedDelegate.Bind(this, [this](const FTransform&)
										 { 
											 this->bStateDirty = true;
										 }, "RigidBody_OnWorldTransformChagned");
	
	//초기 상태 저장
	CachedState.WorldTransform = GetWorldTransform();
	SimulatedState = CachedState;
}

void URigidBodyComponent::Activate()
{
	USceneComponent::Activate();
	RegisterPhysicsSystem();
}

void URigidBodyComponent::DeActivate()
{
	USceneComponent::DeActivate();
	UnRegisterPhysicsSystem();
}

void URigidBodyComponent::Reset()
{
	CachedState = FRigidPhysicsState();
	SimulatedState = CachedState;
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

bool URigidBodyComponent::ShouldSleep() const
{
	return false;
}

#pragma region Helper For Numerical Stability
bool URigidBodyComponent::IsValidForce(const Vector3& InForce)
{
	return InForce.LengthSquared() > 100.0f;
}

bool URigidBodyComponent::IsValidTorque(const Vector3& InTorque)
{
	return InTorque.LengthSquared() > 100.0f;
}

bool URigidBodyComponent::IsValidVelocity(const Vector3& InVelocity)
{
	return InVelocity.LengthSquared() > KINDA_SMALL;
}

bool URigidBodyComponent::IsValidAngularVelocity(const Vector3& InAngularVelocity)
{
	return InAngularVelocity.LengthSquared() > KINDA_SMALL;
}
bool URigidBodyComponent::IsValidAcceleration(const Vector3& InAccel)
{
	return InAccel.LengthSquared() > 1.0f;
}
bool URigidBodyComponent::IsValidAngularAcceleration(const Vector3& InAngularAccel)
{
	return InAngularAccel.LengthSquared() > 1.0f;;
}
#pragma endregion

#pragma region Internal PhysicsState
void URigidBodyComponent::P_UpdateTransformByVelocity(const float DeltaTime)
{
	if (IsStatic())
	{
		return;
	}
	FTransform TargetTransform = SimulatedState.WorldTransform;

	bool bNeedUpdate = false;

	if (IsValidVelocity(SimulatedState.Velocity))
	{
		bNeedUpdate = true;
		// 위치 업데이트
		Vector3 NewPosition = TargetTransform.Position + SimulatedState.Velocity * DeltaTime;
		TargetTransform.Position = NewPosition;
	}

	if (IsValidAngularVelocity(SimulatedState.AngularVelocity))
	{
		bNeedUpdate = true;
		// 회전 업데이트
		Matrix WorldRotation = TargetTransform.GetRotationMatrix();
		XMVECTOR WorldAngularVel = XMLoadFloat3(&SimulatedState.AngularVelocity);
		float AngularSpeed = XMVectorGetX(XMVector3Length(WorldAngularVel));
		if (AngularSpeed > KINDA_SMALL)
		{
			XMVECTOR RotationAxis = XMVector3Normalize(WorldAngularVel);
			float Angle = AngularSpeed * DeltaTime;
			TargetTransform.RotateAroundAxis(
				Vector3(
					XMVectorGetX(RotationAxis),
					XMVectorGetY(RotationAxis),
					XMVectorGetZ(RotationAxis)
				),
				Math::RadToDegree(Angle)
			);
		}
	}
	if (bNeedUpdate)
	{
		P_SetWorldTransform(TargetTransform);
	}
}

void URigidBodyComponent::P_SetWorldPosition(const Vector3& InPoisiton)
{
	if (IsStatic() || !FTransform::IsValidPosition(SimulatedState.WorldTransform.Position - InPoisiton))
	{
		return;
	}
	SimulatedState.WorldTransform.Position = InPoisiton;
}
void URigidBodyComponent::P_SetWorldRotation(const Quaternion& InQuat)
{
	if (IsStatic() || !FTransform::IsValidRotation(SimulatedState.WorldTransform.Rotation, InQuat))
	{
		return;
	}
	SimulatedState.WorldTransform.Rotation = InQuat;
}
void URigidBodyComponent::P_SetWorldScale(const Vector3& InScale)
{
	if (IsStatic() || !FTransform::IsValidScale(SimulatedState.WorldTransform.Scale - InScale))
	{
		return;
	}
	SimulatedState.WorldTransform.Scale = InScale;
}

void URigidBodyComponent::P_ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!IsActive() || IsStatic() || !IsValidForce(Force))
		return;

	SimulatedState.AccumulatedForce += Force;
	Vector3 Torque = Vector3::Cross(Location - GetCenterOfMass(), Force);
	if (IsValidTorque(Torque))
	{
		SimulatedState.AccumulatedTorque += Torque;
	}
}

void URigidBodyComponent::P_ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!IsActive() || IsStatic() || !IsValidForce(Impulse))
		return;
	// 저장된 충격량 처리 (순간적인 속도 변화)
	SimulatedState.Velocity += Impulse  * SimulatedState.InvMass;

	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	if (IsValidTorque(AngularImpulse))
	{
		SimulatedState.AngularVelocity += Vector3(
			AngularImpulse.x * SimulatedState.InvRotationalInertia.x,
			AngularImpulse.y * SimulatedState.InvRotationalInertia.y,
			AngularImpulse.z * SimulatedState.InvRotationalInertia.z);
	}

}

void URigidBodyComponent::P_SetVelocity(const Vector3& InVelocity) 
{
	if (IsStatic() || !IsValidVelocity(SimulatedState.Velocity - InVelocity))
		return;
	SimulatedState.Velocity = InVelocity;
	ClampLinearVelocity(SimulatedState.Velocity);
}

void URigidBodyComponent::P_AddVelocity(const Vector3& InVelocityDelta) 
{
	if (IsStatic() || !IsValidVelocity(InVelocityDelta))
		return;
	SimulatedState.Velocity += InVelocityDelta;
	ClampLinearVelocity(SimulatedState.Velocity);
}

void URigidBodyComponent::P_SetAngularVelocity(const Vector3& InAngularVelocity) 
{
	if (IsStatic() || !IsValidAngularVelocity(SimulatedState.AngularVelocity - InAngularVelocity))
		return;
	SimulatedState.AngularVelocity = InAngularVelocity;
	ClampAngularVelocity(SimulatedState.AngularVelocity);
}

void URigidBodyComponent::P_AddAngularVelocity(const Vector3& InAngularVelocityDelta) 
{
	if (IsStatic() || !IsValidAngularVelocity(InAngularVelocityDelta))
		return;
	SimulatedState.AngularVelocity += InAngularVelocityDelta;
	ClampAngularVelocity(SimulatedState.AngularVelocity);
}
#pragma endregion

#pragma region PhysicsObject Life Cycle
void URigidBodyComponent::TickPhysics(const float DeltaTime)
{
	if (!IsActive() || IsStatic() || IsSleep())
		return;

    //물리 상태
	Vector3 Velocity = SimulatedState.Velocity;
	Vector3 AngularVelocity = SimulatedState.AngularVelocity;
	Vector3 AccumulatedForce = SimulatedState.AccumulatedForce;
	Vector3 AccumulatedTorque = SimulatedState.AccumulatedTorque;

	const float InvMass = SimulatedState.InvMass;
	const Vector3 InvRotationalInertia = SimulatedState.InvRotationalInertia;
	const float FrictionKinetic = SimulatedState.FrictionKinetic;
	const float FrictionStatic = SimulatedState.FrictionStatic;
	const float Restitution = SimulatedState.Restitution;

	// 모든 힘을 가속도로 변환
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


// 시뮬레이션 플래그
void URigidBodyComponent::SetGravity(const bool InBool) 
{
	bGravity = InBool; 
}

void URigidBodyComponent::RegisterPhysicsSystem()
{
	auto myShared = Engine::Cast<IPhysicsObejct>(
		Engine::Cast<URigidBodyComponent>(shared_from_this()));

	assert(myShared, "[Error] InValidPhysicsObejct");
	UPhysicsSystem::Get()->RegisterPhysicsObject(myShared);
}

void URigidBodyComponent::UnRegisterPhysicsSystem()
{
	auto myShared = Engine::Cast<IPhysicsObejct>(
		Engine::Cast<URigidBodyComponent>(shared_from_this()));
	assert(myShared, "[Error] InValidPhysicsObejct");
	UPhysicsSystem::Get()->UnregisterPhysicsObject(myShared);
}

// 내부 연산 결과를 외부용에 반영(동기화)
void URigidBodyComponent::SynchronizeCachedStateFromSimulated()
{
	CachedState = SimulatedState;
	USceneComponent::SetWorldTransform(CachedState.WorldTransform);
	bStateDirty = false;
}
//연산 전 외부 상태 동기화
void URigidBodyComponent::UpdateSimulatedStateFromCached()
{
	if (!bStateDirty)
	{
		return;
	}
	bStateDirty = false;
	FTransform CurrentWorldTransform = USceneComponent::GetWorldTransform();
	CachedState.WorldTransform = CurrentWorldTransform;
	SimulatedState = CachedState;
}

bool URigidBodyComponent::IsDirtyPhysicsState() const
{
	return bStateDirty;
}
#pragma endregion

bool URigidBodyComponent::IsActive() const
{
	return USceneComponent::IsActive();
}

bool URigidBodyComponent::IsSleep() const
{
	return bSleep;
}

void URigidBodyComponent::Sleep()
{
	bSleep = true;
	SimulatedState.Reset();
}

void URigidBodyComponent::Awake()
{
	bSleep = false;
}

#pragma region External PhysicsState

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!IsActive() || IsStatic() || !IsValidForce(Force))
		return;

	bStateDirty = true;

	//CachedState.AccumulatedForce += Force;
	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobApplyForce>(Engine::Cast<URigidBodyComponent>(shared_from_this()),
																 Force,Location);
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);

	//Vector3 Torque = Vector3::Cross(Location - GetCenterOfMass(), Force);
	//if (IsValidTorque(Torque))
	//{
	//	CachedState.AccumulatedTorque += Torque;
	//}
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!IsActive() || IsStatic() || !IsValidForce(Impulse))
		return;

	bStateDirty = true;

	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobApplyImpulse>(Engine::Cast<URigidBodyComponent>(shared_from_this()),
																									 Impulse, Location);
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);

	//CachedState.AccumulatedInstantForce += Impulse;
	//Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	//if (IsValidTorque(AngularImpulse))
	//{
	//	CachedState.AccumulatedInstantTorque += AngularImpulse;
	//}
}

// 물리 속성 설정
void URigidBodyComponent::SetMass(float InMass)
{
	bStateDirty = true;
	CachedState.InvMass = InMass > KINDA_LARGE ? 0.0f : 1 / InMass;

	if (auto Collision = FindChildByType<UCollisionComponentBase>().lock())
	{
		CachedState.InvRotationalInertia = Collision->CalculateInvInertiaTensor(CachedState.InvMass);
	}
	else
	{
		// 회전 관성도 질량에 따라 갱신
		CachedState.InvRotationalInertia = 0.25f * CachedState.InvMass * Vector3::One(); //근사
	}
	
}

void URigidBodyComponent::SetFrictionKinetic(float InFriction)
{
	bStateDirty = true;
	CachedState.FrictionKinetic = InFriction; 
}

void URigidBodyComponent::SetFrictionStatic(float InFriction) 
{
	bStateDirty = true;
	CachedState.FrictionStatic = InFriction;
}

void URigidBodyComponent::SetRestitution(float InRestitution) 
{
	bStateDirty = true;
	CachedState.Restitution = InRestitution; 
}

void URigidBodyComponent::SetRigidType(ERigidBodyType&& InType) 
{ 
	bStateDirty = true;
	CachedState.RigidType = InType;
}

//토큰소유자만 접근 가능
void URigidBodyComponent::SetInvRotationalInertia(const Vector3& Value, const RotationalInertiaToken&)
{ 
	bStateDirty = true;
	CachedState.InvRotationalInertia.x = Value.x > KINDA_LARGE ? 0.0f : 1 / Value.x;
	CachedState.InvRotationalInertia.y = Value.y > KINDA_LARGE ? 0.0f : 1 / Value.y;
	CachedState.InvRotationalInertia.z = Value.z > KINDA_LARGE ? 0.0f : 1 / Value.z;
}

void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
	Vector3 Delta = (CachedState.Velocity - InVelocity);
	if (!IsValidVelocity(Delta))
	{
		return;
	}
	bStateDirty = true;

	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobSetVelocity>(Engine::Cast<URigidBodyComponent>(shared_from_this()),
																  InVelocity);
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);

	//CachedState.Velocity = InVelocity;
	//ClampLinearVelocity(CachedState.Velocity);
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
	if (!IsValidVelocity(InVelocityDelta))
	{
		return;
	}
	bStateDirty = true;
	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobAddVelocity>(Engine::Cast<URigidBodyComponent>(shared_from_this()),
																  InVelocityDelta);
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);


	//CachedState.Velocity += InVelocityDelta;
	//ClampLinearVelocity(CachedState.Velocity);
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
	Vector3 Delta = (CachedState.AngularVelocity - InAngularVelocity);
	if (!IsValidAngularVelocity(Delta))
	{
		return;
	}

	bStateDirty = true;
	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobSetAngularVelocity>(Engine::Cast<URigidBodyComponent>(shared_from_this()),
																  InAngularVelocity);
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);

	//CachedState.AngularVelocity = InAngularVelocity;
	//ClampAngularVelocity(CachedState.AngularVelocity);
}

void URigidBodyComponent::AddAngularVelocity(const Vector3& InAngularVelocityDelta)
{
	if (!IsValidAngularVelocity(InAngularVelocityDelta))
	{
		return;
	}
	bStateDirty = true;
	auto Job = UPhysicsSystem::Get()->AcquireJob<FJobAddAngularVelocity>(Engine::Cast<URigidBodyComponent>(shared_from_this()),
																		 InAngularVelocityDelta);
	UPhysicsSystem::Get()->RequestPhysicsJob(Job);

	//CachedState.AngularVelocity += InAngularVelocityDelta;
	//ClampAngularVelocity(CachedState.AngularVelocity);
}
#pragma endregion

void URigidBodyComponent::ClampVelocities(Vector3& OutVelocity, Vector3& OutAngularVelocity)
{
	ClampLinearVelocity(OutVelocity);
	ClampAngularVelocity(OutAngularVelocity);
}

void URigidBodyComponent::ClampLinearVelocity(Vector3& OutVelocity)
{
	if (IsSpeedRestricted())
	{
		float speedSq = OutVelocity.LengthSquared();
		if (speedSq > MaxSpeed * MaxSpeed)
		{
			OutVelocity = OutVelocity.GetNormalized() * MaxSpeed;
		}
		else if (speedSq < KINDA_SMALL)
		{
			OutVelocity = Vector3::Zero();
		}
	}
}

void URigidBodyComponent::ClampAngularVelocity(Vector3& OutAngularVelocity)
{
	if (IsAngularSpeedRestricted())
	{
		// 각속도 제한
		float angularSpeedSq = OutAngularVelocity.LengthSquared();
		if (angularSpeedSq > MaxAngularSpeed * MaxAngularSpeed)
		{
			OutAngularVelocity = OutAngularVelocity.GetNormalized() * MaxAngularSpeed;
		}
		else if (angularSpeedSq < KINDA_SMALL)
		{
			OutAngularVelocity = Vector3::Zero();
		}
	}
}
