#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

void URigidBodyComponent::Reset()
{
    Velocity = Vector3::Zero;
    AccumulatedForce = Vector3::Zero;
}

const Vector3& URigidBodyComponent::GetCenterOfMass()
{
    //현재는 오너의 위치를 무게중심이라고 가정.
    auto OwnerPtr = Owner.lock();
    assert(OwnerPtr);
    return OwnerPtr->GetTransform()->Position;
}

void URigidBodyComponent::ApplyLinearForce(const Vector3& Force)
{
    XMVECTOR vForce = XMLoadFloat3(&Force);
    XMVECTOR vAccumForce = XMLoadFloat3(&AccumulatedForce);
    XMStoreFloat3(&AccumulatedForce, XMVectorAdd(vAccumForce, vForce));
}

void URigidBodyComponent::ApplyTorque(const Vector3& Torque)
{
    XMVECTOR vTorque = XMLoadFloat3(&Torque);
    XMVECTOR vAccumTorque = XMLoadFloat3(&AccumulatedTorque);
    XMStoreFloat3(&AccumulatedTorque, XMVectorAdd(vAccumTorque, vTorque));
}

void URigidBodyComponent::ClampVelocities()
{
    // 선형 속도 제한
    float speedSq = Velocity.LengthSquared();
    if (speedSq > MaxSpeed * MaxSpeed)
    {
        Velocity = Velocity.GetNormalized() * MaxSpeed;
    }

    // 각속도 제한
    float angularSpeedSq = AngularVelocity.LengthSquared();
    if (angularSpeedSq > MaxAngularSpeed * MaxAngularSpeed)
    {
        AngularVelocity = AngularVelocity.GetNormalized() * MaxAngularSpeed;
    }
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
    if (!bIsSimulatedPhysics)
        return;

#pragma region TEST
    ////TEST
    ////테스트용 가상지면(xz) 마찰력
    //float normalForce = Mass * 0.981f;  // 수직항력
    //float horizontalSpeedSq = Velocity.x * Velocity.x + Velocity.z * Velocity.z;
    //if (horizontalSpeedSq > KINDA_SMALL)
    //{
    //    Vector3 frictionDir = Vector3(-Velocity.x, 0, -Velocity.z);
    //    frictionDir.Normalize();

    //    float frictionMagnitude = normalForce * FrictionKinetic;

    //    Vector3 friction = frictionDir * frictionMagnitude;
    //    ApplyForce(friction);
    //}
    ////테스트용 가상지면 회전 마찰력
    //// 회전 마찰력
    //float angularSpeedSq = AngularVelocity.LengthSquared();
    //if (angularSpeedSq > KINDA_SMALL)
    //{
    //    Vector3 angularFrictionDir = -AngularVelocity.GetNormalized();
    //    float angularFrictionMagnitude = normalForce * FrictionKinetic * RotationalInertia;

    //    Vector3 angularFriction = angularFrictionDir * angularFrictionMagnitude;
    //    ApplyTorque(angularFriction);
    //}
#pragma endregion

    if (bGravity)
    {
        ApplyLinearForce(GravityDirection * GravityScale * Mass);
    }

    // SIMD 최적화를 위한 벡터 로드
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMVECTOR vAngularVel = XMLoadFloat3(&AngularVelocity);
    XMVECTOR vAccumForce = XMLoadFloat3(&AccumulatedForce);
    XMVECTOR vAccumTorque = XMLoadFloat3(&AccumulatedTorque);


    // 가속도 계산 (a = F/m)
    XMVECTOR vAcceleration = XMVectorScale(vAccumForce, 1.0f / Mass);
    XMVECTOR vAngularAccel = XMVectorScale(vAccumTorque, 1.0f / RotationalInertia);

    // 속도 갱신
    vVelocity = XMVectorAdd(vVelocity, XMVectorScale(vAcceleration, DeltaTime));
    vAngularVel = XMVectorAdd(vAngularVel, XMVectorScale(vAngularAccel, DeltaTime));

    // 속도 제한
    XMStoreFloat3(&Velocity, vVelocity);
    XMStoreFloat3(&AngularVelocity, vAngularVel);
    ClampVelocities();
    vVelocity = XMLoadFloat3(&Velocity);
    vAngularVel = XMLoadFloat3(&AngularVelocity);

    // 상태 업데이트
    if (auto OwnerPtr = Owner.lock())
    {
        // 위치 업데이트
        XMVECTOR vPosition = XMLoadFloat3(&OwnerPtr->GetTransform()->Position);
        vPosition = XMVectorAdd(vPosition, XMVectorScale(vVelocity, DeltaTime));

        // 회전 업데이트 (각속도로부터 쿼터니언 변화 계산)
        float angularSpeed = XMVectorGetX(XMVector3Length(vAngularVel));
        if (angularSpeed > KINDA_SMALL)
        {
            XMVECTOR rotationAxis = XMVector3Normalize(vAngularVel);
            XMVECTOR deltaRotation = XMQuaternionRotationNormal(rotationAxis, angularSpeed * DeltaTime);
            XMVECTOR currentRotation = XMLoadFloat4(&OwnerPtr->GetTransform()->Rotation);
            XMVECTOR newRotation = XMQuaternionMultiply(currentRotation, deltaRotation);
            newRotation = XMQuaternionNormalize(newRotation);

            // 결과 저장
            Quaternion finalRotation;
            XMStoreFloat4(&finalRotation, newRotation);
            OwnerPtr->SetRotationQuaternion(finalRotation);
        }

        Vector3 newPosition;
        XMStoreFloat3(&newPosition, vPosition);
        OwnerPtr->SetPosition(newPosition);
    }
  

    // 누적값 초기화
    AccumulatedForce = Vector3::Zero;
    AccumulatedTorque = Vector3::Zero;
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& ApplyPosition)
{
    if (!bIsSimulatedPhysics) 
        return;

    // 무게중심 기준 위치로 변환
    Vector3 relativePos = ApplyPosition - GetCenterOfMass();

    // 선형력 적용
    ApplyLinearForce(Force);

    // 위치에 따른 회전력 계산 및 적용
    if (relativePos.LengthSquared() > KINDA_SMALL)
    {
        Vector3 torque = Vector3::Cross(relativePos, Force);
        ApplyTorque(torque);
    }
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Position)
{
    if (!bIsSimulatedPhysics) return;

    // 즉각적인 선형 속도 변화
    XMVECTOR vImpulse = XMLoadFloat3(&Impulse);
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMStoreFloat3(&Velocity, XMVectorAdd(vVelocity, XMVectorScale(vImpulse, 1.0f / Mass)));

    // 무게중심 기준 위치로 변환
    Vector3 relativePos = Position - GetCenterOfMass();

    // 위치가 다르다면 각운동량 변화
    if (relativePos.LengthSquared() > KINDA_SMALL)
    {
        Vector3 angularImpulse = Vector3::Cross(relativePos, Impulse);
        XMVECTOR vAngularImpulse = XMLoadFloat3(&angularImpulse);
        XMVECTOR vAngularVel = XMLoadFloat3(&AngularVelocity);
        XMStoreFloat3(&AngularVelocity,
                      XMVectorAdd(vAngularVel,
                                  XMVectorScale(vAngularImpulse, 1.0f / RotationalInertia)));
    }

    ClampVelocities();
}