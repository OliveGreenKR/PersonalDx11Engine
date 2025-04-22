#include "SphereComponent.h"
#include "DebugDrawerManager.h"
#include "SceneManager.h"

Vector3 USphereComponent::GetSupportPoint(const Vector3& Direction) const
{
    XMVECTOR Dir = XMLoadFloat3(&Direction);
    XMVECTOR NormDir = XMVector3Normalize(Dir);
    XMVECTOR Center = XMLoadFloat3(&GetWorldTransform().Position);
    XMVECTOR Support = XMVectorAdd(Center, XMVectorScale(NormDir, GetHalfExtent().x));

    Vector3 Result;
    XMStoreFloat3(&Result, Support);
    return Result;
}

Vector3 USphereComponent::CalculateInertiaTensor(float Mass) const
{
    // For a solid sphere: I = (2/5) * m * r^2
    float r = GetHalfExtent().x;
    float r2 = r * r;
    float Inertia = (2.0f / 5.0f) * Mass * r2;

    return Vector3(Inertia, Inertia, Inertia);
}

void USphereComponent::CalculateAABB(Vector3& OutMin, Vector3& OutMax) const
{
    XMVECTOR Center = XMLoadFloat3(&GetWorldTransform().Position);
    XMVECTOR Extent = XMVectorReplicate(GetHalfExtent().x); // (r, r, r, r)

    XMVECTOR MinPoint = XMVectorSubtract(Center, Extent);
    XMVECTOR MaxPoint = XMVectorAdd(Center, Extent);

    XMStoreFloat3(&OutMin, MinPoint);
    XMStoreFloat3(&OutMax, MaxPoint);
}
