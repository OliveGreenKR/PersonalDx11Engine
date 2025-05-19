#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "Debug.h"
#include "PhysicsDefine.h"

URigidBodyComponent::URigidBodyComponent()
{
	bPhysicsSimulated = true;
}

void URigidBodyComponent::Reset()
{
	CachedState = FRigidPhysicsState();
	CurrentState = CachedState;
}	

void URigidBodyComponent::Tick(const float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);
	if (!IsActive())
		return;
	SynchronizeState();

	// 물리 상태
	Vector3 Velocity = CurrentState.Velocity;
	Vector3 AngularVelocity = CurrentState.AngularVelocity;
	Vector3 AccumulatedForce = CurrentState.AccumulatedForce;
	Vector3 AccumulatedTorque = CurrentState.AccumulatedTorque;
	Vector3 AccumulatedInstantForce = CurrentState.AccumulatedInstantForce;
	Vector3 AccumulatedInstantTorque = CurrentState.AccumulatedInstantTorque;

	// 물리 속성
	float Mass = CurrentState.Mass;
	Vector3 RotationalInertia = CurrentState.RotationalInertia;
	float FrictionKinetic = CurrentState.FrictionKinetic;
	float FrictionStatic = CurrentState.FrictionStatic;
	float Restitution = CurrentState.Restitution;
	float LinearDrag = CurrentState.LinearDrag;
	float AngularDrag = CurrentState.AngularDrag;

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
	if (Velocity.LengthSquared() > KINDA_SMALL)
	{

		// 정적 마찰력 영역에서 운동 마찰력 영역으로의 전환 확인
		if (Velocity.Length() < KINDA_SMALL &&
			AccumulatedForce.Length() <= FrictionStatic * Mass * GravityFactor)
		{
			// 정적 마찰력이 외력을 상쇄
			AccumulatedForce = Vector3::Zero();
		}
		else
		{
			// 운동 마찰력 적용
			Vector3 frictionAccel = -Velocity.GetNormalized() * FrictionKinetic * GravityFactor;
			TotalAcceleration += frictionAccel;
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
	Velocity += TotalAcceleration * DeltaTime;
	AngularVelocity += TotalAngularAcceleration * DeltaTime;

	//if (AccumulatedTorque.LengthSquared() > KINDA_SMALL * KINDA_SMALL)
	//{
	//	LOG("AngularVelocity with AccumulatedTorque : %s", Debug::ToString(AngularVelocity));
	//	LOG("*------- AccumTorque : %s", Debug::ToString(AccumulatedTorque));
	//}
		

	// 속도 제한
	ClampVelocities(Velocity, AngularVelocity);

	// 위치 업데이트
	UpdateTransform(DeltaTime);

	// 외부 힘 초기화
	AccumulatedForce = Vector3::Zero();
	AccumulatedTorque = Vector3::Zero();

	// 상태 업데이트
    CurrentState.Velocity = Velocity;
    CurrentState.AngularVelocity = AngularVelocity;
    CurrentState.AccumulatedForce = AccumulatedForce;
    CurrentState.AccumulatedTorque = AccumulatedTorque;
    CurrentState.AccumulatedInstantForce = AccumulatedInstantForce;
    CurrentState.AccumulatedInstantTorque = AccumulatedInstantTorque;

    // 물리 속성
    CurrentState.Mass = Mass;
    CurrentState.RotationalInertia = RotationalInertia;
    CurrentState.FrictionKinetic = FrictionKinetic;
    CurrentState.FrictionStatic = FrictionStatic;
    CurrentState.Restitution = Restitution;
    CurrentState.LinearDrag = LinearDrag;
    CurrentState.AngularDrag = AngularDrag;

	CaptureState();
}

void URigidBodyComponent::TickPhysics(const float DeltaTime)
{
	LOG_FUNC_CALL("URigidBody TickPhysics with [%.3f] seconds", DeltaTime);
}

void URigidBodyComponent::UpdateTransform(const float DeltaTime)
{
	if (RigidType == ERigidBodyType::Static)
	{
		return;
	}
	FTransform TargetTransform = GetWorldTransform();

	if (CurrentState.Velocity.Length() > KINDA_SMALLER)
	{
		// 위치 업데이트
		Vector3 NewPosition = TargetTransform.Position + CurrentState.Velocity * DeltaTime;
		TargetTransform.Position = NewPosition;
	}
	
	// 회전 업데이트
	Matrix WorldRotation = TargetTransform.GetRotationMatrix();
	XMVECTOR WorldAngularVel = XMLoadFloat3(&CurrentState.AngularVelocity);
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
	SetWorldTransform(TargetTransform);
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!IsActive())
		return;

	CachedState.AccumulatedForce += Force;
	Vector3 Torque = Vector3::Cross(Location - GetCenterOfMass(), Force);
	CachedState.AccumulatedTorque += Torque;
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!IsActive())
		return;

	CachedState.AccumulatedInstantForce += Impulse;
	Vector3 COM = GetCenterOfMass();
	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	CachedState.AccumulatedInstantTorque += AngularImpulse;
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	return GetWorldTransform().Position;
}


void URigidBodyComponent::SynchronizeState()
{
	if (IsDirty())
	{
		LOG_FUNC_CALL("[Error] Invalid Trial for Synchronization of Physics States.");
		return;
	}
	CurrentState = CachedState;
	bStateDirty = false;
}

void URigidBodyComponent::CaptureState() const
{
	CachedState = CurrentState;
}

bool URigidBodyComponent::IsDirty() const
{
	return bStateDirty;
}

bool URigidBodyComponent::IsActive() const
{
	return USceneComponent::IsActive();
}


void URigidBodyComponent::SetMass(float InMass)
{
	CachedState.Mass = std::max(InMass, KINDA_SMALL);
	// 회전 관성도 질량에 따라 갱신
	CachedState.RotationalInertia = 4.0f * CachedState.Mass * Vector3::One(); //근사
}

void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
	CachedState.Velocity = InVelocity;
	ClampLinearVelocity(CachedState.Velocity);
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
	CachedState.Velocity += InVelocityDelta;
	ClampLinearVelocity(CachedState.Velocity);
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
	CachedState.AngularVelocity = InAngularVelocity;
	ClampAngularVelocity(CachedState.AngularVelocity);
}

void URigidBodyComponent::AddAngularVelocity(const Vector3& InAngularVelocityDelta)
{
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
