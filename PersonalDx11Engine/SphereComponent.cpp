#include "SphereComponent.h"
#include "DebugDrawerManager.h"
#include "PhysicsDefine.h"

Vector3 USphereComponent::GetWorldSupportPoint(const Vector3& WorldDirection) const
{
    // 입력 방향 확인 및 정규화
    XMVECTOR Dir = XMLoadFloat3(&WorldDirection);
    if (XMVector3LengthSq(Dir).m128_f32[0] < KINDA_SMALL)
    {
        Dir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    }
    Dir = XMVector3Normalize(Dir);

    float Radius = GetScaledHalfExtent().x;
    XMVECTOR WorldRadius = Radius * Dir;
    Vector3 RadiusVec;
    XMStoreFloat3(&RadiusVec, WorldRadius);
    return GetWorldPosition()+RadiusVec;
}

Vector3 USphereComponent::CalculateInertiaTensor(float Mass) const
{
    // For a solid sphere: I = (2/5) * m * r^2
    float r = GetScaledHalfExtent().x;
    float r2 = r * r;
    float Inertia = (2.0f / 5.0f) * Mass * r2;

    return Vector3(Inertia, Inertia, Inertia);
}

void USphereComponent::CalculateAABB(Vector3& OutMin, Vector3& OutMax) const
{
    XMVECTOR Center = XMLoadFloat3(&GetWorldTransform().Position);
    XMVECTOR Extent = XMVectorReplicate(GetScaledHalfExtent().x); // (r, r, r, r)

    XMVECTOR MinPoint = XMVectorSubtract(Center, Extent);
    XMVECTOR MaxPoint = XMVectorAdd(Center, Extent);

    XMStoreFloat3(&OutMin, MinPoint);
    XMStoreFloat3(&OutMax, MaxPoint);
}

void USphereComponent::RequestDebugRender(const float DeltaTime)
{
    UDebugDrawManager::Get()->DrawSphere(
        GetWorldPosition(),
        GetWorldScale().x * 0.5f,
        GetWorldRotation(),
        Vector4(1, 1, 0, 1),
        DeltaTime
    );
}
