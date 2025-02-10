#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

void URigidBodyComponent::Reset()
{
	LinearVelocity = Vector3::Zero;
	AngularVelocity = Vector3::Zero;
	LinearAcceleration = Vector3::Zero;
	AngularAcceleration = Vector3::Zero;
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
	if (!bIsSimulatedPhysics)
		return;

	if (bGravity)
	{
		AddLinearAcceleration(GravityDirection * GravityScale);
	}

	UpdateLinearVelocity(DeltaTime);
	UpdateAngularVelocity(DeltaTime);
	UpdateTransform(DeltaTime);

	// 가속도 초기화
	LinearAcceleration = Vector3::Zero;
	AngularAcceleration = Vector3::Zero;
}

void URigidBodyComponent::UpdateLinearVelocity(const float DeltaTime)
{
	LinearVelocity += LinearAcceleration * DeltaTime;

	// 마찰력에 의한 감속
	if (LinearVelocity.LengthSquared() > KINDA_SMALL)
	{
		Vector3 frictionDir = -LinearVelocity.GetNormalized();
		Vector3 frictionAcc = frictionDir * FrictionKinetic * GravityScale;
		LinearVelocity += frictionAcc * DeltaTime;
	}

	ClampVelocities();
}

void URigidBodyComponent::UpdateAngularVelocity(const float DeltaTime)
{
	AngularVelocity += AngularAcceleration * DeltaTime;

	// 회전 마찰력
	if (AngularVelocity.LengthSquared() > KINDA_SMALL)
	{
		Vector3 frictionDir = -AngularVelocity.GetNormalized();
		Vector3 frictionAcc = frictionDir * FrictionKinetic * GravityScale;
		AngularVelocity += frictionAcc * DeltaTime;
	}

	ClampVelocities();
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

	AddLinearAcceleration(Force / Mass);

	Vector3 Torque = Vector3::Cross(Location - GetCenterOfMass(), Force);
	AddAngularAcceleration(Torque / RotationalInertia);
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!bIsSimulatedPhysics)
		return;

	AddLinearVelocity(Impulse / Mass);

	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	AddAngularVelocity(AngularImpulse / RotationalInertia);
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