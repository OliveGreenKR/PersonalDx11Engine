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
    //����� ������ ��ġ�� �����߽��̶�� ����.
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
    // ���� �ӵ� ����
    float speedSq = Velocity.LengthSquared();
    if (speedSq > MaxSpeed * MaxSpeed)
    {
        Velocity = Velocity.GetNormalized() * MaxSpeed;
    }

    // ���ӵ� ����
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
    ////�׽�Ʈ�� ��������(xz) ������
    //float normalForce = Mass * 0.981f;  // �����׷�
    //float horizontalSpeedSq = Velocity.x * Velocity.x + Velocity.z * Velocity.z;
    //if (horizontalSpeedSq > KINDA_SMALL)
    //{
    //    Vector3 frictionDir = Vector3(-Velocity.x, 0, -Velocity.z);
    //    frictionDir.Normalize();

    //    float frictionMagnitude = normalForce * FrictionKinetic;

    //    Vector3 friction = frictionDir * frictionMagnitude;
    //    ApplyForce(friction);
    //}
    ////�׽�Ʈ�� �������� ȸ�� ������
    //// ȸ�� ������
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

    // SIMD ����ȭ�� ���� ���� �ε�
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMVECTOR vAngularVel = XMLoadFloat3(&AngularVelocity);
    XMVECTOR vAccumForce = XMLoadFloat3(&AccumulatedForce);
    XMVECTOR vAccumTorque = XMLoadFloat3(&AccumulatedTorque);


    // ���ӵ� ��� (a = F/m)
    XMVECTOR vAcceleration = XMVectorScale(vAccumForce, 1.0f / Mass);
    XMVECTOR vAngularAccel = XMVectorScale(vAccumTorque, 1.0f / RotationalInertia);

    // �ӵ� ����
    vVelocity = XMVectorAdd(vVelocity, XMVectorScale(vAcceleration, DeltaTime));
    vAngularVel = XMVectorAdd(vAngularVel, XMVectorScale(vAngularAccel, DeltaTime));

    // �ӵ� ����
    XMStoreFloat3(&Velocity, vVelocity);
    XMStoreFloat3(&AngularVelocity, vAngularVel);
    ClampVelocities();
    vVelocity = XMLoadFloat3(&Velocity);
    vAngularVel = XMLoadFloat3(&AngularVelocity);

    // ���� ������Ʈ
    if (auto OwnerPtr = Owner.lock())
    {
        // ��ġ ������Ʈ
        XMVECTOR vPosition = XMLoadFloat3(&OwnerPtr->GetTransform()->Position);
        vPosition = XMVectorAdd(vPosition, XMVectorScale(vVelocity, DeltaTime));

        // ȸ�� ������Ʈ (���ӵ��κ��� ���ʹϾ� ��ȭ ���)
        float angularSpeed = XMVectorGetX(XMVector3Length(vAngularVel));
        if (angularSpeed > KINDA_SMALL)
        {
            XMVECTOR rotationAxis = XMVector3Normalize(vAngularVel);
            XMVECTOR deltaRotation = XMQuaternionRotationNormal(rotationAxis, angularSpeed * DeltaTime);
            XMVECTOR currentRotation = XMLoadFloat4(&OwnerPtr->GetTransform()->Rotation);
            XMVECTOR newRotation = XMQuaternionMultiply(currentRotation, deltaRotation);
            newRotation = XMQuaternionNormalize(newRotation);

            // ��� ����
            Quaternion finalRotation;
            XMStoreFloat4(&finalRotation, newRotation);
            OwnerPtr->SetRotationQuaternion(finalRotation);
        }

        Vector3 newPosition;
        XMStoreFloat3(&newPosition, vPosition);
        OwnerPtr->SetPosition(newPosition);
    }
  

    // ������ �ʱ�ȭ
    AccumulatedForce = Vector3::Zero;
    AccumulatedTorque = Vector3::Zero;
}

void URigidBodyComponent::ApplyForce(const Vector3& Force, const Vector3& ApplyPosition)
{
    if (!bIsSimulatedPhysics) 
        return;

    // �����߽� ���� ��ġ�� ��ȯ
    Vector3 relativePos = ApplyPosition - GetCenterOfMass();

    // ������ ����
    ApplyLinearForce(Force);

    // ��ġ�� ���� ȸ���� ��� �� ����
    if (relativePos.LengthSquared() > KINDA_SMALL)
    {
        Vector3 torque = Vector3::Cross(relativePos, Force);
        ApplyTorque(torque);
    }
}

void URigidBodyComponent::ApplyImpulse(const Vector3& Impulse, const Vector3& Position)
{
    if (!bIsSimulatedPhysics) return;

    // �ﰢ���� ���� �ӵ� ��ȭ
    XMVECTOR vImpulse = XMLoadFloat3(&Impulse);
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMStoreFloat3(&Velocity, XMVectorAdd(vVelocity, XMVectorScale(vImpulse, 1.0f / Mass)));

    // �����߽� ���� ��ġ�� ��ȯ
    Vector3 relativePos = Position - GetCenterOfMass();

    // ��ġ�� �ٸ��ٸ� ����� ��ȭ
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