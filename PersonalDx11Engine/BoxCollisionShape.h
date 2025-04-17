#pragma once
#include "CollisionShapeInterface.h"
#include <DirectXMath.h>
class UBoxCollisionShape : public ICollisionShape
{
public:
    UBoxCollisionShape(const Vector3& InHalfExtent = Vector3::One * 0.5f)
        : HalfExtent(InHalfExtent)
    {
    }
    //주어진 방향의 가장 먼 월드 위치 반환
    virtual Vector3 GetSupportPoint(const Vector3& Direction, const FTransform& WorldTransform) const override
    {
        // 로컬 방향으로 변환
        XMMATRIX RotationMatrix = WorldTransform.GetRotationMatrix();
        XMMATRIX InvRotation = XMMatrixInverse(nullptr, RotationMatrix);
        XMVECTOR LocalDir = XMVector3TransformNormal(XMLoadFloat3(&Direction), InvRotation);

        // 부호에 따라 지원점 계산
        XMVECTOR SupportExtent = XMLoadFloat3(&HalfExtent);
        XMVECTOR SignMask = XMVectorGreaterOrEqual(LocalDir, XMVectorZero()); // d_i >= 0 ? 0xFFFFFFFF : 0
        XMVECTOR LocalSupport = XMVectorSelect(XMVectorNegate(SupportExtent), SupportExtent, SignMask); // d_i >= 0 ? +h_i : -h_i

        // 월드 공간으로 변환
        XMMATRIX ModelingMatrix = WorldTransform.GetModelingMatrix();
        XMVECTOR WorldSupport = XMVector3TransformCoord(LocalSupport, ModelingMatrix);

        // 결과 저장
        Vector3 Result;
        XMStoreFloat3(&Result, WorldSupport);
        return Result;
    }

    virtual Vector3 CalculateInertiaTensor(float Mass) const override
    {
        // Load half extents into SIMD vector
        XMVECTOR vHalfExtent = XMLoadFloat3(&HalfExtent); 

        // Square each component: he * he
        XMVECTOR vSquared = XMVectorMultiply(vHalfExtent, vHalfExtent); // (x², y², z²)

        // Shuffle to get the proper sum combinations for inertia tensor
        // I.x = (y² + z²)
        // I.y = (x² + z²)
        // I.z = (x² + y²)

        XMVECTOR tensor = XMVectorSet(
            XMVectorGetY(vSquared) + XMVectorGetZ(vSquared), // y² + z²
            XMVectorGetX(vSquared) + XMVectorGetZ(vSquared), // x² + z²
            XMVectorGetX(vSquared) + XMVectorGetY(vSquared), // x² + y²
            0.0f
        );

        // Scale with mass * (1/12)
        float Scale = Mass / 12.0f;
        XMVECTOR vInerteria = XMVectorScale(tensor, Scale);

        // Store result
        Vector3 Result;
        XMStoreFloat3(&Result, vInerteria);
        return Result;
    }

    virtual void CalculateAABB(const FTransform& WorldTransform, Vector3& OutMin, Vector3& OutMax) const override
    {
        // Load rotation matrix once
        XMMATRIX WorldRotateMatrix = WorldTransform.GetRotationMatrix();
        XMVECTOR Position = XMLoadFloat3(&WorldTransform.Position);

        // Preload HalfExtent
        float hx = HalfExtent.x;
        float hy = HalfExtent.y;
        float hz = HalfExtent.z;

        // Precomputed 8 local corners in SIMD directly
        XMVECTOR Points[8];
        Points[0] = XMVectorSet(-hx, -hy, -hz, 0.0f);
        Points[1] = XMVectorSet(+hx, -hy, -hz, 0.0f);
        Points[2] = XMVectorSet(-hx, +hy, -hz, 0.0f);
        Points[3] = XMVectorSet(+hx, +hy, -hz, 0.0f);
        Points[4] = XMVectorSet(-hx, -hy, +hz, 0.0f);
        Points[5] = XMVectorSet(+hx, -hy, +hz, 0.0f);
        Points[6] = XMVectorSet(-hx, +hy, +hz, 0.0f);
        Points[7] = XMVectorSet(+hx, +hy, +hz, 0.0f);

        // Apply rotation
        for (int i = 0; i < 8; ++i)
            Points[i] = XMVector3Transform(Points[i], WorldRotateMatrix);

        // Initialize Min/Max with first rotated point
        XMVECTOR MinPoint = Points[0];
        XMVECTOR MaxPoint = Points[0];

        for (int i = 1; i < 8; ++i)
        {
            MinPoint = XMVectorMin(MinPoint, Points[i]);
            MaxPoint = XMVectorMax(MaxPoint, Points[i]);
        }

        // Add world position in SIMD
        MinPoint = XMVectorAdd(MinPoint, Position);
        MaxPoint = XMVectorAdd(MaxPoint, Position);

        // Store back
        XMStoreFloat3(&OutMin, MinPoint);
        XMStoreFloat3(&OutMax, MaxPoint);
    }

    // 박스 전용 접근자
    const Vector3& GetHalfExtent() const override { return HalfExtent; }
    void SetHalfExtent(const Vector3& InHalfExtent) { HalfExtent = InHalfExtent; }

private:
    //로컬 공간의 extent
    Vector3 HalfExtent;
};