#include "SphereComponent.h"

Vector3 USphereComponent::GetSupportPoint(const Vector3& Direction, const FTransform& WorldTransform) const
{
    XMVECTOR Dir = XMLoadFloat3(&Direction);
    XMVECTOR NormDir = XMVector3Normalize(Dir);
    XMVECTOR Center = XMLoadFloat3(&WorldTransform.Position);
    XMVECTOR Support = XMVectorAdd(Center, XMVectorScale(NormDir, HalfExtent.x));

    Vector3 Result;
    XMStoreFloat3(&Result, Support);
    return Result;
}

Vector3 USphereComponent::CalculateInertiaTensor(float Mass) const
{
    // For a solid sphere: I = (2/5) * m * r^2
    float r2 = HalfExtent.x * HalfExtent.x;
    float Inertia = (2.0f / 5.0f) * Mass * r2;

    return Vector3(Inertia, Inertia, Inertia);
}

void USphereComponent::CalculateAABB(const FTransform& WorldTransform, Vector3& OutMin, Vector3& OutMax) const
{
    XMVECTOR Center = XMLoadFloat3(&WorldTransform.Position);
    XMVECTOR Extent = XMVectorReplicate(HalfExtent.x); // (r, r, r, r)

    XMVECTOR MinPoint = XMVectorSubtract(Center, Extent);
    XMVECTOR MaxPoint = XMVectorAdd(Center, Extent);

    XMStoreFloat3(&OutMin, MinPoint);
    XMStoreFloat3(&OutMax, MaxPoint);
}
