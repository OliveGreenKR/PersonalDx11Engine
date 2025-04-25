#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"
#include "Debug.h"

URigidBodyComponent::URigidBodyComponent()
{
}

void URigidBodyComponent::Reset()
{
	Velocity = Vector3::Zero;
	AngularVelocity = Vector3::Zero;
	AccumulatedForce = Vector3::Zero;
	AccumulatedTorque = Vector3::Zero;
	AccumulatedInstantForce = Vector3::Zero;
	AccumulatedInstantTorque = Vector3::Zero;
}	

void URigidBodyComponent::Tick(const float DeltaTime)
{
	USceneComponent::Tick(DeltaTime);
	if (!IsActive())
		return;

	// 모든 힘을 가속도로 변환
	Vector3 TotalAcceleration = Vector3::Zero;
	Vector3 TotalAngularAcceleration = Vector3::Zero;

	// 중력 가속도 추가
	if (bGravity)
	{
		TotalAcceleration += GravityDirection * GravityScale;
	}

	//마찰력
	if (Velocity.LengthSquared() > KINDA_SMALL)
	{

		// 정적 마찰력 영역에서 운동 마찰력 영역으로의 전환 확인
		if (Velocity.Length() < KINDA_SMALL &&
			AccumulatedForce.Length() <= FrictionStatic * Mass * GravityScale )
		{
			// 정적 마찰력이 외력을 상쇄
			AccumulatedForce = Vector3::Zero;
		}
		else
		{
			// 운동 마찰력 적용
			Vector3 frictionAccel = -Velocity.GetNormalized() * FrictionKinetic * GravityScale;
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

	// 충격량 초기화
	AccumulatedInstantForce = Vector3::Zero;
	AccumulatedInstantTorque = Vector3::Zero;

	// 외부에서 적용된 힘에 의한 가속도 추가
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += Vector3(
		AccumulatedTorque.x / RotationalInertia.x,
		AccumulatedTorque.y / RotationalInertia.y,
		AccumulatedTorque.z / RotationalInertia.z);

	// 통합된 가속도로 속도 업데이트
	Velocity += TotalAcceleration * DeltaTime;
	AngularVelocity += TotalAngularAcceleration * DeltaTime;

	// 속도 제한
	ClampVelocities();

	// 위치 업데이트
	UpdateTransform(DeltaTime);

	// 외부 힘 초기화
	AccumulatedForce = Vector3::Zero;
	AccumulatedTorque = Vector3::Zero;
}

void URigidBodyComponent::UpdateTransform(const float DeltaTime)
{
	if (RigidType == ERigidBodyType::Static)
	{
		return;
	}

	FTransform TargetTransform = GetWorldTransform();

	// 위치 업데이트
	Vector3 NewPosition = TargetTransform.Position + Velocity * DeltaTime;
	TargetTransform.Position = NewPosition;

	// 회전 업데이트
	Matrix WorldRotation = TargetTransform.GetRotationMatrix();
	XMVECTOR WorldAngularVel = XMLoadFloat3(&AngularVelocity);

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

	AccumulatedForce += Force;
	AccumulatedTorque += Vector3::Cross(Location - GetCenterOfMass(), Force);
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!IsActive())
		return;

	AccumulatedInstantForce += Impulse;
	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	AccumulatedInstantTorque += AngularImpulse;
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	return GetWorldTransform().Position;
}


void URigidBodyComponent::SetMass(float InMass)
{
	Mass = std::max(InMass, KINDA_SMALL);
	// 회전 관성도 질량에 따라 갱신
	RotationalInertia = 4.0f * Mass * Vector3::One; //근사
}

void URigidBodyComponent::SetVelocity(const Vector3& InVelocity)
{
	Velocity = InVelocity;
	ClampVelocities();
}

void URigidBodyComponent::AddVelocity(const Vector3& InVelocityDelta)
{
	Velocity += InVelocityDelta;
	ClampVelocities();
}

void URigidBodyComponent::SetAngularVelocity(const Vector3& InAngularVelocity)
{
	AngularVelocity = InAngularVelocity;
	ClampVelocities();
}

void URigidBodyComponent::AddAngularVelocity(const Vector3& InAngularVelocityDelta)
{
	AngularVelocity += InAngularVelocityDelta;
	ClampVelocities();
}

void URigidBodyComponent::ClampVelocities()
{
	// 선형 속도 제한
	if (IsSpeedRestricted())
	{
		float speedSq = Velocity.LengthSquared();
		if (speedSq > MaxSpeed * MaxSpeed)
		{
			Velocity = Velocity.GetNormalized() * MaxSpeed;
		}
		else if (speedSq < KINDA_SMALL)
		{
			Velocity = Vector3::Zero;
		}
	}
	
	if (IsAngularSpeedRestricted())
	{
		// 각속도 제한
		float angularSpeedSq = AngularVelocity.LengthSquared();
		if (angularSpeedSq > MaxAngularSpeed * MaxAngularSpeed)
		{
			AngularVelocity = AngularVelocity.GetNormalized() * MaxAngularSpeed;
		}
		else if (angularSpeedSq < KINDA_SMALL)
		{
			AngularVelocity = Vector3::Zero;
		}
	}
}