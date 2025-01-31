#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

void URigidBodyComponent::Reset()
{
    Velocity = Vector3::Zero;
    AccumulatedForce = Vector3::Zero;
}

void URigidBodyComponent::Tick(const float DeltaTime)
{
    if (!bIsSimulatedPhysics)
        return;

    // SIMD ����ȭ�� ���� ���� �ε�
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMVECTOR vAccumForce = XMLoadFloat3(&AccumulatedForce);
    Vector3 gravity = GravityScale * Gravity * bUseGravity;
    XMVECTOR vGravity = XMLoadFloat3(&gravity);

    // ������ ��� (v�� �ݴ� ����)
    XMVECTOR vFriction = XMVectorZero();
    if (XMVectorGetX(XMVector3LengthSq(vVelocity)) > KINDA_SMALL)
    {
        vFriction = XMVectorScale(
            XMVector3Normalize(XMVectorNegate(vVelocity)),
            FrictionCoefficient
        );
    }

    // ���ӵ� ���: a = F/m
    XMVECTOR vAcceleration = XMVectorAdd(
        XMVectorScale(vAccumForce, 1.0f / Mass),  // �ܷ¿� ���� ���ӵ�
        XMVectorAdd(vGravity, vFriction)          // �߷� + ������
    );

    // �ӵ� ����: v = v0 + at
    vVelocity = XMVectorAdd(
        vVelocity,
        XMVectorScale(vAcceleration, DeltaTime)
    );

    // �ִ� �ӵ� ����
    float CurrentSpeedSq = XMVectorGetX(XMVector3LengthSq(vVelocity));
    if (CurrentSpeedSq > MaxSpeed * MaxSpeed)
    {
        vVelocity = XMVectorScale(
            XMVector3Normalize(vVelocity),
            MaxSpeed
        );
    }

    // �ӵ��� �ſ� ������ ����
    if (CurrentSpeedSq < KINDA_SMALL)
    {
        vVelocity = XMVectorZero();
    }

    //// ��ġ ����: x = x0 + vt
    //if (auto OwnerPtr = Owner.lock())
    //{
    //    XMVECTOR vPosition = XMLoadFloat3(&OwnerPtr->GetTransform()->Position);
    //    vPosition = XMVectorAdd(vPosition, XMVectorScale(vVelocity, DeltaTime));

    //    Vector3 NewPosition;
    //    XMStoreFloat3(&NewPosition, vPosition);
    //    OwnerPtr->SetPosition(NewPosition);
    //}

    // ��� ����
    XMStoreFloat3(&Velocity, vVelocity);
    AccumulatedForce = Vector3::Zero; 

}

void URigidBodyComponent::ApplyForce(const Vector3& Force)
{
    XMVECTOR vForce = XMLoadFloat3(&Force);
    XMVECTOR vAccumForce = XMLoadFloat3(&AccumulatedForce);
    XMStoreFloat3(&AccumulatedForce, XMVectorAdd(vAccumForce, vForce));
}

void URigidBodyComponent::ApplyImpulse(const Vector3& InImpulse)
{
    XMVECTOR vImpulse = XMLoadFloat3(&InImpulse);
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMStoreFloat3(&Velocity, XMVectorAdd(vVelocity, XMVectorScale(vImpulse, 1.0f / Mass)));
}
