#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

void URigidBodyComponent::Reset()
{
	LinearVelocity = Vector3::Zero;
	AngularVelocity = Vector3::Zero;
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
	if (!bIsSimulatedPhysics)
		return;

	// 1. 모든 힘을 가속도로 변환하여 하나의 순수한 가속도 벡터로 관리
	Vector3 TotalAcceleration = Vector3::Zero;
	Vector3 TotalAngularAcceleration = Vector3::Zero;

	// 중력 가속도 추가
	if (bGravity)
	{
		TotalAcceleration += GravityDirection * GravityScale;
	}

	// 마찰력을 가속도로 변환하여 추가
	if (LinearVelocity.LengthSquared() > KINDA_SMALL)
	{
		Vector3 FrictionAccel = -LinearVelocity.GetNormalized() * (FrictionKinetic * GravityScale);
		TotalAcceleration += FrictionAccel;
	}

	// 외부에서 적용된 힘에 의한 가속도 추가
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += AccumulatedTorque / RotationalInertia;

	// 2. 통합된 가속도로 속도 업데이트
	LinearVelocity += TotalAcceleration * DeltaTime;
	AngularVelocity += TotalAngularAcceleration * DeltaTime;

	// 3. 속도 제한
	ClampVelocities();

	// 4. 위치 업데이트
	UpdateTransform(DeltaTime);

	// 5. 외부 가속도 초기화
	AccumulatedForce = Vector3::Zero;
	AccumulatedTorque = Vector3::Zero;
}

void URigidBodyComponent::UpdateTransform(const float DeltaTime)
{
	if (auto OwnerPtr = Owner.lock())
	{
		// 위치 업데이트
		Vector3 NewPosition = OwnerPtr->GetTransform()->Position + LinearVelocity * DeltaTime;
		OwnerPtr->SetPosition(NewPosition);

		// 회전 업데이트
		float AngularSpeed = AngularVelocity.Length();
		if (AngularSpeed > KINDA_SMALL)
		{
			Vector3 RotationAxis = AngularVelocity.GetNormalized();
			float AngleDegrees = Math::RadToDegree(AngularSpeed * DeltaTime);
			OwnerPtr->GetTransform()->RotateAroundAxis(RotationAxis, AngleDegrees);
		}
	}
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!bIsSimulatedPhysics)
		return;

	AccumulatedForce += Force;
	AccumulatedTorque += Vector3::Cross(Location - GetCenterOfMass(), Force);
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!bIsSimulatedPhysics)
		return;

	static constexpr float ImpulseDuration = 0.016f;
	Vector3 InstantForce = Impulse / ImpulseDuration;

	AccumulatedInstantForce += InstantForce;
	AccumulatedInstantTorque += Vector3::Cross(Location - GetCenterOfMass(), InstantForce);
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	auto OwnerPtr = Owner.lock();
	assert(OwnerPtr);
	return OwnerPtr->GetTransform()->Position;
}

void URigidBodyComponent::SetMass(float InMass)
{
	Mass = std::max(InMass, KINDA_SMALL);
	// 회전 관성도 질량에 따라 갱신
	RotationalInertia = Mass * 0.4f; // 구체 근사
}

void URigidBodyComponent::SetLinearVelocity(const Vector3& InVelocity)
{
	LinearVelocity = InVelocity;
	ClampVelocities();
}

void URigidBodyComponent::AddLinearVelocity(const Vector3& InVelocityDelta)
{
	LinearVelocity += InVelocityDelta;
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
	float speedSq = LinearVelocity.LengthSquared();
	if (speedSq > MaxSpeed * MaxSpeed)
	{
		LinearVelocity = LinearVelocity.GetNormalized() * MaxSpeed;
	}

	// 각속도 제한
	float angularSpeedSq = AngularVelocity.LengthSquared();
	if (angularSpeedSq > MaxAngularSpeed * MaxAngularSpeed)
	{
		AngularVelocity = AngularVelocity.GetNormalized() * MaxAngularSpeed;
	}
}