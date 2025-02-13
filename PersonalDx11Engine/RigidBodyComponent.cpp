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

		//정적 마찰 비교
		if (AngularVelocity.Length() < KINDA_SMALL && 
			AccumulatedTorque.Length() <= FrictionStatic * RotationalInertia)
		{
			// 정적 마찰 토크가 외부 토크를 상쇄
			AccumulatedTorque = Vector3::Zero;
		}
		else
		{
			// 운동 마찰 토크 적용
			Vector3 frictionAccel = -AngularVelocity * FrictionKinetic;
			TotalAngularAcceleration += frictionAccel;
		}
	}


	// 저장된 충격량 처리 (순간적인 속도 변화)
	Velocity += AccumulatedInstantForce / Mass;
	AngularVelocity += AccumulatedInstantTorque / RotationalInertia;

	// 충격량 초기화
	AccumulatedInstantForce = Vector3::Zero;
	AccumulatedInstantTorque = Vector3::Zero;

	// 외부에서 적용된 힘에 의한 가속도 추가
	TotalAcceleration += AccumulatedForce / Mass;
	TotalAngularAcceleration += AccumulatedTorque / RotationalInertia;

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
	if (auto OwnerPtr = Owner.lock())
	{
		// 위치 업데이트
		Vector3 NewPosition = OwnerPtr->GetTransform()->GetPosition() + Velocity * DeltaTime;
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
	// 회전 관성도 질량에 따라 갱신
	RotationalInertia = 1.0f * Mass; //근사
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
	float speedSq = Velocity.LengthSquared();
	if (speedSq > MaxSpeed * MaxSpeed)
	{
		Velocity = Velocity.GetNormalized() * MaxSpeed;
	}
	else if (speedSq < KINDA_SMALL)
	{
		Velocity = Vector3::Zero;
	}

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