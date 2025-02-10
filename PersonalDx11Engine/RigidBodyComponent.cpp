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

	// 1. ��� ���� ���ӵ��� ��ȯ�Ͽ� �ϳ��� ������ ���ӵ� ���ͷ� ����
	Vector3 TotalAcceleration = Vector3::Zero;
	Vector3 TotalAngularAcceleration = Vector3::Zero;

	// �߷� ���ӵ� �߰�
	if (bGravity)
	{
		TotalAcceleration += GravityDirection * GravityScale;
	}

	// �������� ���ӵ��� ��ȯ�Ͽ� �߰�
	if (LinearVelocity.LengthSquared() > KINDA_SMALL)
	{
		Vector3 FrictionAccel = -LinearVelocity.GetNormalized() * (FrictionKinetic * GravityScale);
		TotalAcceleration += FrictionAccel;
	}

	// �ܺο��� ����� ���� ���� ���ӵ� �߰�
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += AccumulatedTorque / RotationalInertia;

	// 2. ���յ� ���ӵ��� �ӵ� ������Ʈ
	LinearVelocity += TotalAcceleration * DeltaTime;
	AngularVelocity += TotalAngularAcceleration * DeltaTime;

	// 3. �ӵ� ����
	ClampVelocities();

	// 4. ��ġ ������Ʈ
	UpdateTransform(DeltaTime);

	// 5. �ܺ� ���ӵ� �ʱ�ȭ
	AccumulatedForce = Vector3::Zero;
	AccumulatedTorque = Vector3::Zero;
}

void URigidBodyComponent::UpdateTransform(const float DeltaTime)
{
	if (auto OwnerPtr = Owner.lock())
	{
		// ��ġ ������Ʈ
		Vector3 NewPosition = OwnerPtr->GetTransform()->Position + LinearVelocity * DeltaTime;
		OwnerPtr->SetPosition(NewPosition);

		// ȸ�� ������Ʈ
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
	// ȸ�� ������ ������ ���� ����
	RotationalInertia = Mass * 0.4f; // ��ü �ٻ�
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
	// ���� �ӵ� ����
	float speedSq = LinearVelocity.LengthSquared();
	if (speedSq > MaxSpeed * MaxSpeed)
	{
		LinearVelocity = LinearVelocity.GetNormalized() * MaxSpeed;
	}

	// ���ӵ� ����
	float angularSpeedSq = AngularVelocity.LengthSquared();
	if (angularSpeedSq > MaxAngularSpeed * MaxAngularSpeed)
	{
		AngularVelocity = AngularVelocity.GetNormalized() * MaxAngularSpeed;
	}
}