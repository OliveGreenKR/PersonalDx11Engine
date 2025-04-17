#pragma once
#include "CollisionShapeInterface.h"

using namespace DirectX;

class USphereCollisionShape : public ICollisionShape
{
public:
    USphereCollisionShape(float Radius = 1.0f)
        : HalfExtent(Radius,Radius,Radius) {
    }

    virtual ~USphereCollisionShape() = default;

    virtual Vector3 GetSupportPoint(const Vector3& Direction, const FTransform& WorldTransform) const override
    {
        XMVECTOR Dir = XMLoadFloat3(&Direction);
        XMVECTOR NormDir = XMVector3Normalize(Dir);
        XMVECTOR Center = XMLoadFloat3(&WorldTransform.Position);
        XMVECTOR Support = XMVectorAdd(Center, XMVectorScale(NormDir, HalfExtent.x));

        Vector3 Result;
        XMStoreFloat3(&Result, Support);
        return Result;
    }

    virtual Vector3 CalculateInertiaTensor(float Mass) const override
    {
        // For a solid sphere: I = (2/5) * m * r^2
        float r2 = HalfExtent.x * HalfExtent.x;
        float Inertia = (2.0f / 5.0f) * Mass * r2;

        return Vector3(Inertia, Inertia, Inertia);
    }

    virtual void CalculateAABB(const FTransform& WorldTransform, Vector3& OutMin, Vector3& OutMax) const override
    {
        XMVECTOR Center = XMLoadFloat3(&WorldTransform.Position);
        XMVECTOR Extent = XMVectorReplicate(HalfExtent.x); // (r, r, r, r)

        XMVECTOR MinPoint = XMVectorSubtract(Center, Extent);
        XMVECTOR MaxPoint = XMVectorAdd(Center, Extent);

        XMStoreFloat3(&OutMin, MinPoint);
        XMStoreFloat3(&OutMax, MaxPoint);
    }

    //원의 경우 x 성분이 반지름
    virtual const Vector3& GetHalfExtent() const override
    {
        return HalfExtent;
    }

    //원의 경우 x 성분이 반지름
    virtual void SetHalfExtent(const Vector3& InVector) override
    {
        HalfExtent = InVector;
    }

private:
    Vector3 HalfExtent; 
};