#include "RigidBodyComponent.h"
#include "Transform.h"
#include "GameObject.h"

void URigidBodyComponent::Tick(float DeltaTime)
{
    if (!bSimulatePhysics) return;

    // SIMD 최적화를 위해 XMVECTOR 사용
    XMVECTOR vVelocity = XMLoadFloat3(&Velocity);
    XMVECTOR vAcceleration = XMLoadFloat3(&Acceleration);
    XMVECTOR vDeltaTime = XMVectorReplicate(DeltaTime);

    // v = v0 + at
    vVelocity = XMVectorAdd(vVelocity, XMVectorMultiply(vAcceleration, vDeltaTime));
    XMStoreFloat3(&Velocity, vVelocity);

    if (Owner.lock())
    {
        auto TransformPtr = Owner.lock()->GetTransform();
        // x = x0 + vt
        XMVECTOR vPosition = XMLoadFloat3(&TransformPtr->Position);
        vPosition = XMVectorAdd(vPosition, XMVectorMultiply(vVelocity, vDeltaTime));
        Vector3 NewPosition;
        XMStoreFloat3(&NewPosition, vPosition);
        Owner.lock()->SetPosition(NewPosition);
    }
}

void URigidBodyComponent::ApplyForce(const Vector3& Force)
{
    // F = ma
    XMVECTOR vForce = XMLoadFloat3(&Force);
    XMVECTOR vAccel = XMVectorScale(vForce, 1.0f / Mass);
    XMVECTOR vCurrentAccel = XMLoadFloat3(&Acceleration);
    XMStoreFloat3(&Acceleration, XMVectorAdd(vCurrentAccel, vAccel));
}
