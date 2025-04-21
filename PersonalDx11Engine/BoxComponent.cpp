#include "BoxComponent.h"

Vector3 UBoxComponent::GetSupportPoint(const Vector3& Direction) const
{
    // 로컬 방향으로 변환
    XMMATRIX RotationMatrix = GetWorldTransform().GetRotationMatrix();
    XMMATRIX InvRotation = XMMatrixInverse(nullptr, RotationMatrix);
    XMVECTOR LocalDir = XMVector3Normalize(XMVector3Transform(XMLoadFloat3(&Direction), InvRotation));
    

    // 부호에 따라 지원점 계산
    XMVECTOR SupportExtent = XMLoadFloat3(&HalfExtent);
    XMVECTOR SignMask = XMVectorGreaterOrEqual(LocalDir, XMVectorZero()); // d_i >= 0 ? 0xFFFFFFFF : 0
    XMVECTOR LocalSupport = XMVectorSelect(XMVectorNegate(SupportExtent), SupportExtent, SignMask); // d_i >= 0 ? +h_i : -h_i

    // 월드 공간으로 변환
    XMMATRIX ModelingMatrix = GetWorldTransform().GetModelingMatrix();
    XMVECTOR WorldSupport = XMVector3TransformCoord(LocalSupport, ModelingMatrix);

    // 결과 저장
    Vector3 Result;
    XMStoreFloat3(&Result, WorldSupport);
    return Result;
}

Vector3 UBoxComponent::CalculateInertiaTensor(float Mass) const
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

void UBoxComponent::CalculateAABB(Vector3& OutMin, Vector3& OutMax) const
{
    // Load rotation matrix once
    XMMATRIX WorldRotateMatrix = GetWorldTransform().GetRotationMatrix();
    XMVECTOR Position = XMLoadFloat3(&GetWorldTransform().Position);

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
