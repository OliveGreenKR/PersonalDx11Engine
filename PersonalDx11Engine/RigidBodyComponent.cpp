#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

URigidBodyComponent::URigidBodyComponent()
{
	SetSimulatePhysics(true);
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
	UPrimitiveComponent::Tick(DeltaTime);

	if (!GetSimulatePhysics())
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
		//�� �ະ�� ���� ���� �˻�
		bool bStaticFrictionX = std::abs(AngularVelocity.x) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.x) <= FrictionStatic * RotationalInertia.x;
		bool bStaticFrictionY = std::abs(AngularVelocity.y) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.y) <= FrictionStatic * RotationalInertia.y;
		bool bStaticFrictionZ = std::abs(AngularVelocity.z) < KINDA_SMALL &&
			std::abs(AccumulatedTorque.z) <= FrictionStatic * RotationalInertia.z;

		// �ະ�� ����/� ���� ����
		Vector3 frictionAccel;
		frictionAccel.x = bStaticFrictionX ? -AccumulatedTorque.x : -AngularVelocity.x * FrictionKinetic;
		frictionAccel.y = bStaticFrictionY ? -AccumulatedTorque.y : -AngularVelocity.y * FrictionKinetic;
		frictionAccel.z = bStaticFrictionZ ? -AccumulatedTorque.z : -AngularVelocity.z * FrictionKinetic;

		TotalAngularAcceleration += frictionAccel;
	}


	// ����� ��ݷ� ó�� (�������� �ӵ� ��ȭ)
	Velocity += AccumulatedInstantForce / Mass;
	AngularVelocity += Vector3(
		AccumulatedInstantTorque.x / RotationalInertia.x,
		AccumulatedInstantTorque.y / RotationalInertia.y,
		AccumulatedInstantTorque.z / RotationalInertia.z);

	// ��ݷ� �ʱ�ȭ
	AccumulatedInstantForce = Vector3::Zero;
	AccumulatedInstantTorque = Vector3::Zero;

	// �ܺο��� ����� ���� ���� ���ӵ� �߰�
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += Vector3(
		AccumulatedTorque.x / RotationalInertia.x,
		AccumulatedTorque.y / RotationalInertia.y,
		AccumulatedTorque.z / RotationalInertia.z);

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
	if (RigidType == ERigidBodyType::Static)
	{
		return;
	}

	FTransform* TargetTransform = nullptr;
	if (bSyncWithOwner || GetOwner())
	{
		TargetTransform = GetOwner()->GetTransform();
	}
	else
	{
		TargetTransform = &ComponentTransform;
	}
	

	// ��ġ ������Ʈ
	Vector3 NewPosition = TargetTransform->GetPosition() + Velocity * DeltaTime;
	TargetTransform->SetPosition(NewPosition);

	// ȸ�� ������Ʈ
	Matrix WorldRotation = TargetTransform->GetRotationMatrix();
	XMVECTOR WorldAngularVel = XMLoadFloat3(&AngularVelocity);

	float AngularSpeed = XMVectorGetX(XMVector3Length(WorldAngularVel));
	if (AngularSpeed > KINDA_SMALL)
	{
		XMVECTOR RotationAxis = XMVector3Normalize(WorldAngularVel);
		float Angle = AngularSpeed * DeltaTime;
		TargetTransform->RotateAroundAxis(
			Vector3(
				XMVectorGetX(RotationAxis),
				XMVectorGetY(RotationAxis),
				XMVectorGetZ(RotationAxis)
			),
			Math::RadToDegree(Angle)
		);
	}
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& Location)
{
	if (!GetSimulatePhysics())
		return;

	AccumulatedForce += Force;
	AccumulatedTorque += Vector3::Cross(Location - GetCenterOfMass(), Force);
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Location)
{
	if (!GetSimulatePhysics())
		return;

	AccumulatedInstantForce += Impulse;
	Vector3 AngularImpulse = Vector3::Cross(Location - GetCenterOfMass(), Impulse);
	AccumulatedInstantTorque += AngularImpulse;
}

Vector3 URigidBodyComponent::GetCenterOfMass() const
{
	return GetTransform()->GetPosition();
}

const FTransform* URigidBodyComponent::GetTransform() const
{
	if (bSyncWithOwner || GetOwner())
	{
		return  GetOwner()->GetTransform();
	}
	else
	{
		return &ComponentTransform;
	}
}

FTransform* URigidBodyComponent::GetTransform()
{
	if (bSyncWithOwner || GetOwner())
	{
		return  GetOwner()->GetTransform();
	}
	else
	{
		return &ComponentTransform;
	}
}

void URigidBodyComponent::SetMass(float InMass)
{
	Mass = std::max(InMass, KINDA_SMALL);
	// ȸ�� ������ ������ ���� ����
	RotationalInertia = 4.0f * Mass * Vector3::One; //�ٻ�
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