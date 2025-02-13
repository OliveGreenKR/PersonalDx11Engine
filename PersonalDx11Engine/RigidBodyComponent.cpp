#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

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
	UActorComponent::Tick(DeltaTime);

	if (!bIsSimulatedPhysics)
		return;

	// ��� ���� ���ӵ��� ��ȯ
	Vector3 TotalAcceleration = Vector3::Zero;
	Vector3 TotalAngularAcceleration = Vector3::Zero;

	// �߷� ���ӵ� �߰�
	if (bGravity)
	{
		TotalAcceleration += GravityDirection * GravityScale;
	}

	//������
	if (Velocity.LengthSquared() > KINDA_SMALL)
	{

		// ���� ������ �������� � ������ ���������� ��ȯ Ȯ��
		if (Velocity.Length() < KINDA_SMALL &&
			AccumulatedForce.Length() <= FrictionStatic * Mass * GravityScale )
		{
			// ���� �������� �ܷ��� ���
			AccumulatedForce = Vector3::Zero;
		}
		else
		{
			// � ������ ����
			Vector3 frictionAccel = -Velocity.GetNormalized() * FrictionKinetic * GravityScale;
			TotalAcceleration += frictionAccel;
		}
	}

	// ��� ������ ó��
	if (AngularVelocity.LengthSquared() > KINDA_SMALL)
	{

		//���� ���� ��
		if (AngularVelocity.Length() < KINDA_SMALL && 
			AccumulatedTorque.Length() <= FrictionStatic * RotationalInertia)
		{
			// ���� ���� ��ũ�� �ܺ� ��ũ�� ���
			AccumulatedTorque = Vector3::Zero;
		}
		else
		{
			// � ���� ��ũ ����
			Vector3 frictionAccel = -AngularVelocity * FrictionKinetic;
			TotalAngularAcceleration += frictionAccel;
		}
	}


	// ����� ��ݷ� ó�� (�������� �ӵ� ��ȭ)
	Velocity += AccumulatedInstantForce / Mass;
	AngularVelocity += AccumulatedInstantTorque / RotationalInertia;

	// ��ݷ� �ʱ�ȭ
	AccumulatedInstantForce = Vector3::Zero;
	AccumulatedInstantTorque = Vector3::Zero;

	// �ܺο��� ����� ���� ���� ���ӵ� �߰�
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += AccumulatedTorque / RotationalInertia;

	// ���յ� ���ӵ��� �ӵ� ������Ʈ
	Velocity += TotalAcceleration * DeltaTime;
	AngularVelocity += TotalAngularAcceleration * DeltaTime;

	// �ӵ� ����
	ClampVelocities();

	// ��ġ ������Ʈ
	UpdateTransform(DeltaTime);

	// �ܺ� �� �ʱ�ȭ
	AccumulatedForce = Vector3::Zero;
	AccumulatedTorque = Vector3::Zero;
}

void URigidBodyComponent::UpdateTransform(const float DeltaTime)
{
	if (auto OwnerPtr = Owner.lock())
	{
		// ��ġ ������Ʈ
		Vector3 NewPosition = OwnerPtr->GetTransform()->GetPosition() + Velocity * DeltaTime;
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

	AccumulatedInstantForce += Impulse;
	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	AccumulatedInstantTorque += AngularImpulse;
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	auto OwnerPtr = Owner.lock();
	assert(OwnerPtr);
	return OwnerPtr->GetTransform()->GetPosition();
}

const UGameObject* URigidBodyComponent::GetOwner()
{
	return Owner.lock() ? Owner.lock().get() : nullptr ;
}

const UActorComponent* URigidBodyComponent::GetOwnerComponent()
{
	return nullptr;
}

const FTransform* URigidBodyComponent::GetTransform() const
{
	if (auto OwnerPtr = Owner.lock())
	{
		return  OwnerPtr->GetTransform();
	}
	return nullptr;
}

void URigidBodyComponent::SetMass(float InMass)
{
	Mass = std::max(InMass, KINDA_SMALL);
	// ȸ�� ������ ������ ���� ����
	RotationalInertia = 1.0f * Mass; //�ٻ�
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
	// ���� �ӵ� ����
	float speedSq = Velocity.LengthSquared();
	if (speedSq > MaxSpeed * MaxSpeed)
	{
		Velocity = Velocity.GetNormalized() * MaxSpeed;
	}
	else if (speedSq < KINDA_SMALL)
	{
		Velocity = Vector3::Zero;
	}

	// ���ӵ� ����
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