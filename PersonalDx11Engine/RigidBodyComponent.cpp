#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "Debug.h"
#include "PhysicsDefine.h"
#include "PhysicsSystem.h"
#include "TypeCast.h"


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

void URigidBodyComponent::TickPhysics(const float DeltaTime)
{
	if (!IsActive())
		return;

	// 물리 상태 - TODO : for test, Directly CachedState
	Vector3& Velocity = SimulatedState.Velocity;
	Vector3& AngularVelocity = SimulatedState.AngularVelocity;
 	Vector3& AccumulatedForce = SimulatedState.AccumulatedForce;
	Vector3& AccumulatedTorque = SimulatedState.AccumulatedTorque;
	Vector3& AccumulatedInstantForce = SimulatedState.AccumulatedInstantForce;
	Vector3& AccumulatedInstantTorque = SimulatedState.AccumulatedInstantTorque;

	float& Mass = SimulatedState.Mass;
	Vector3& RotationalInertia = SimulatedState.RotationalInertia;
	float& FrictionKinetic = SimulatedState.FrictionKinetic;
	float& FrictionStatic = SimulatedState.FrictionStatic;
	float& Restitution = SimulatedState.Restitution;

	// 모든 힘을 가속도로 변환
	Vector3 TotalAcceleration = Vector3::Zero();
	Vector3 TotalAngularAcceleration = Vector3::Zero();

	float GravityFactor = GravityScale * ONE_METER;
	// 중력 가속도 추가
	if (bGravity)
	{
		TotalAcceleration += GravityDirection * GravityFactor;
	}

	//마찰력
	Vector3 frictionAccel = Vector3::Zero();
	if (Velocity.LengthSquared() > KINDA_SMALL)
	{

		// 정적 마찰력 영역에서 운동 마찰력 영역으로의 전환 확인
		if (Velocity.Length() < KINDA_SMALL &&
			AccumulatedForce.Length() <= FrictionStatic * Mass * GravityFactor)
		{
			// 정적 마찰력이 외력을 상쇄
			AccumulatedForce = Vector3::Zero();
			Velocity = Vector3::Zero();
		}
		else
		{
			// 운동 마찰력 계산
			frictionAccel = -Velocity.GetNormalized() * FrictionKinetic * GravityFactor;
		}
	}
	//마찰 제한 적용 -  운동마찰은 객체의 속도를 0까지만 만들 수 있음
	if (frictionAccel.LengthSquared() > KINDA_SMALL * KINDA_SMALL)
	{
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

	// 각운동 마찰력 처리
	if (AngularVelocity.LengthSquared() > KINDA_SMALL)
	{
		//각 축별로 정적 마찰 검사
		bool bStaticFrictionX = std::abs(AngularVelocity.x) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.x) <= FrictionStatic * RotationalInertia.x;
		bool bStaticFrictionY = std::abs(AngularVelocity.y) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.y) <= FrictionStatic * RotationalInertia.y;
		bool bStaticFrictionZ = std::abs(AngularVelocity.z) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.z) <= FrictionStatic * RotationalInertia.z;

		// 축별로 정적/운동 마찰 적용
		Vector3 frictionAccel;
		frictionAccel.x = bStaticFrictionX ? -AccumulatedTorque.x : -AngularVelocity.x * FrictionKinetic;
		frictionAccel.y = bStaticFrictionY ? -AccumulatedTorque.y : -AngularVelocity.y * FrictionKinetic;
		frictionAccel.z = bStaticFrictionZ ? -AccumulatedTorque.z : -AngularVelocity.z * FrictionKinetic;

		TotalAngularAcceleration += frictionAccel;
	}

	// 저장된 충격량 처리 (순간적인 속도 변화)
	Velocity += AccumulatedInstantForce / Mass;
	AngularVelocity += Vector3(
		AccumulatedInstantTorque.x / RotationalInertia.x,
		AccumulatedInstantTorque.y / RotationalInertia.y,
		AccumulatedInstantTorque.z / RotationalInertia.z);
	//if (AccumulatedInstantTorque.LengthSquared() > KINDA_SMALL * KINDA_SMALL)
	//{
	//	LOG("AngularVelocity with InstantTorque : %s", Debug::ToString(AngularVelocity));
	//	LOG("*------- InstantTorque : %s", Debug::ToString(AccumulatedInstantTorque));
	//}

	// 충격량 초기화
	AccumulatedInstantForce = Vector3::Zero();
	AccumulatedInstantTorque = Vector3::Zero();

	// 외부에서 적용된 힘에 의한 가속도 추가
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += Vector3(
		AccumulatedTorque.x / RotationalInertia.x,
		AccumulatedTorque.y / RotationalInertia.y,
		AccumulatedTorque.z / RotationalInertia.z);

	// 통합된 가속도로 속도 업데이트
	if (TotalAcceleration.LengthSquared() > KINDA_SMALL * KINDA_SMALL)
	{
		Velocity += TotalAcceleration * DeltaTime;
	}
	
	if (TotalAngularAcceleration.LengthSquared() > KINDA_SMALLER * KINDA_SMALLER)
	{
		AngularVelocity += TotalAngularAcceleration * DeltaTime;
	}
	
	//if (AccumulatedTorque.LengthSquared() > KINDA_SMALL * KINDA_SMALL)
	//{
	//	LOG("AngularVelocity with AccumulatedTorque : %s", Debug::ToString(AngularVelocity));
	//	LOG("*------- AccumTorque : %s", Debug::ToString(AccumulatedTorque));
	//}

	// 속도 제한
	ClampVelocities(Velocity, AngularVelocity);

	// 위치 업데이트
	P_UpdateTransform(DeltaTime);

	// 외부 힘 초기화
	AccumulatedForce = Vector3::Zero();
	AccumulatedTorque = Vector3::Zero();
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

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!IsActive())
		return;

	bStateDirty = true;

	CachedState.AccumulatedForce += Force;
	Vector3 Torque = Vector3::Cross(Location - GetCenterOfMass(), Force);
	CachedState.AccumulatedTorque += Torque;
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!IsActive())
		return;
	bStateDirty = true;

	CachedState.AccumulatedInstantForce += Impulse;
	Vector3 COM = GetCenterOfMass();
	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	CachedState.AccumulatedInstantTorque += AngularImpulse;
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	return GetWorldTransform().Position;
}

#pragma region P_Regions
void URigidBodyComponent::P_UpdateTransform(const float DeltaTime)
{
	if (SimulatedState.RigidType == ERigidBodyType::Static)
	{
		return;
	}
	FTransform TargetTransform = SimulatedState.WorldTransform;

	if (SimulatedState.Velocity.Length() > KINDA_SMALLER)
	{
		// 위치 업데이트
		Vector3 NewPosition = TargetTransform.Position + SimulatedState.Velocity * DeltaTime;
		TargetTransform.Position = NewPosition;
	}

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
	P_SetWorldTransform(TargetTransform);
}

void URigidBodyComponent::P_SetWorldPosition(const Vector3& InPoisiton)
{
	constexpr float TRANSFORM_EPSILON = KINDA_SMALLER;

	if ((SimulatedState.WorldTransform.Position - InPoisiton).LengthSquared() > TRANSFORM_EPSILON * TRANSFORM_EPSILON)
	{
		SimulatedState.WorldTransform.Position = InPoisiton;
	}
}
void URigidBodyComponent::P_SetWorldRotation(const Quaternion& InQuat)
{
	constexpr float TRANSFORM_EPSILON = KINDA_SMALLER;

	// 변화량이 의미 있는지 확인
	float Dot = Quaternion::Dot(SimulatedState.WorldTransform.Rotation, InQuat);
	float ChangeMagnitude = std::abs(1.0f - std::abs(Dot));

	if (ChangeMagnitude > TRANSFORM_EPSILON)
	{
		SimulatedState.WorldTransform.Rotation = InQuat;
	}

}
void URigidBodyComponent::P_SetWorldScale(const Vector3& InScale)
{
	constexpr float TRANSFORM_EPSILON = KINDA_SMALLER;

	if ((SimulatedState.WorldTransform.Scale - InScale).LengthSquared() > TRANSFORM_EPSILON * TRANSFORM_EPSILON)
	{
		SimulatedState.WorldTransform.Scale = InScale;
	}
}
void URigidBodyComponent::P_ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!IsActive())
		return;

	SimulatedState.AccumulatedForce += Force;
	Vector3 Torque = Vector3::Cross(Location - GetCenterOfMass(), Force);
	SimulatedState.AccumulatedTorque += Torque;
}
void URigidBodyComponent::P_ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!IsActive())
		return;

	SimulatedState.AccumulatedInstantForce += Impulse;
	Vector3 COM = GetCenterOfMass();
	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	SimulatedState.AccumulatedInstantTorque += AngularImpulse;
}
void URigidBodyComponent::P_SetVelocity(const Vector3& InVelocity) 
{
	SimulatedState.Velocity = InVelocity;
	ClampLinearVelocity(SimulatedState.Velocity);
}
void URigidBodyComponent::P_AddVelocity(const Vector3& InVelocityDelta) 
{
	SimulatedState.Velocity += InVelocityDelta;
	ClampLinearVelocity(SimulatedState.Velocity);
}
void URigidBodyComponent::P_SetAngularVelocity(const Vector3& InAngularVelocity) 
{
	SimulatedState.AngularVelocity = InAngularVelocity;
	ClampAngularVelocity(SimulatedState.AngularVelocity);
}
void URigidBodyComponent::P_AddAngularVelocity(const Vector3& InAngularVelocityDelta) 
{
	SimulatedState.AngularVelocity += InAngularVelocityDelta;
	ClampAngularVelocity(CachedState.AngularVelocity);
}
#pragma endregion
// 내부 연산 결과를 외부용에 반영(동기화)
void URigidBodyComponent::SynchronizeCachedStateFromSimulated()
{
	CachedState = SimulatedState;
	SetWorldTransform(CachedState.WorldTransform);
	
}
//연산 전 외부 상태 동기화
void URigidBodyComponent::UpdateSimulatedStateFromCached()
{
	bStateDirty = false;
	FTransform CurrentWorldTransform = GetWorldTransform();
	CachedState.WorldTransform = CurrentWorldTransform;
	SimulatedState = CachedState;
}

bool URigidBodyComponent::IsDirtyPhysicsState() const
{
	return bStateDirty;
}

bool URigidBodyComponent::IsActive() const
{
	return USceneComponent::IsActive();
}


// 물리 속성 설정
void URigidBodyComponent::SetMass(float InMass)
{
	bStateDirty = true;
	CachedState.Mass = std::max(InMass, KINDA_SMALL);
	// 회전 관성도 질량에 따라 갱신
	CachedState.RotationalInertia = 4.0f * CachedState.Mass * Vector3::One(); //근사
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
void URigidBodyComponent::SetRotationalInertia(const Vector3& Value, const RotationalInertiaToken&) 
{ 
	bStateDirty = true;
	CachedState.RotationalInertia = Value;
}

void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
	bStateDirty = true;
	CachedState.Velocity = InVelocity;
	ClampLinearVelocity(CachedState.Velocity);
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
	bStateDirty = true;
	CachedState.Velocity += InVelocityDelta;
	ClampLinearVelocity(CachedState.Velocity);
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
	bStateDirty = true;
	CachedState.AngularVelocity = InAngularVelocity;
	ClampAngularVelocity(CachedState.AngularVelocity);
}

void URigidBodyComponent::AddAngularVelocity(const Vector3& InAngularVelocityDelta)
{
	bStateDirty = true;
	CachedState.AngularVelocity += InAngularVelocityDelta;
	ClampAngularVelocity(CachedState.AngularVelocity);
}

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
