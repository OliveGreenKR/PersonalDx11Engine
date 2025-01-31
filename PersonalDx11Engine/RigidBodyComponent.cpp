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

    // SIMD 최적화를 위한 벡터 로드
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMVECTOR vAccumForce = XMLoadFloat3(&AccumulatedForce);
    Vector3 gravity = GravityScale * Gravity * bUseGravity;
    XMVECTOR vGravity = XMLoadFloat3(&gravity);

    // 마찰력 계산 (v의 반대 방향)
    XMVECTOR vFriction = XMVectorZero();
    if (XMVectorGetX(XMVector3LengthSq(vVelocity)) > KINDA_SMALL)
    {
        vFriction = XMVectorScale(
            XMVector3Normalize(XMVectorNegate(vVelocity)),
            FrictionCoefficient
        );
    }

    // 가속도 계산: a = F/m
    XMVECTOR vAcceleration = XMVectorAdd(
        XMVectorScale(vAccumForce, 1.0f / Mass),  // 외력에 의한 가속도
        XMVectorAdd(vGravity, vFriction)          // 중력 + 마찰력
    );

    // 속도 갱신: v = v0 + at
    vVelocity = XMVectorAdd(
        vVelocity,
        XMVectorScale(vAcceleration, DeltaTime)
    );

    // 최대 속도 제한
    float CurrentSpeedSq = XMVectorGetX(XMVector3LengthSq(vVelocity));
    if (CurrentSpeedSq > MaxSpeed * MaxSpeed)
    {
        vVelocity = XMVectorScale(
            XMVector3Normalize(vVelocity),
            MaxSpeed
        );
    }

    // 속도가 매우 작으면 정지
    if (CurrentSpeedSq < KINDA_SMALL)
    {
        vVelocity = XMVectorZero();
    }

    //// 위치 갱신: x = x0 + vt
    //if (auto OwnerPtr = Owner.lock())
    //{
    //    XMVECTOR vPosition = XMLoadFloat3(&OwnerPtr->GetTransform()->Position);
    //    vPosition = XMVectorAdd(vPosition, XMVectorScale(vVelocity, DeltaTime));

    //    Vector3 NewPosition;
    //    XMStoreFloat3(&NewPosition, vPosition);
    //    OwnerPtr->SetPosition(NewPosition);
    //}

    // 결과 저장
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
