#include "CollisionComponent.h"
#include "GameObject.h"

#pragma region BoundingVolume
bool FBoundingVolume::IsIntersectBox(const FBoundingVolume& Other) const
{
    XMVECTOR vCenter = XMLoadFloat3(&Center);
    XMVECTOR vExtents = XMLoadFloat3(&Extents);
    XMVECTOR vOtherCenter = XMLoadFloat3(&Other.Center);
    XMVECTOR vOtherExtents = XMLoadFloat3(&Other.Extents);

    XMVECTOR vDelta = XMVectorAbs(XMVectorSubtract(vCenter, vOtherCenter));
    XMVECTOR vSum = XMVectorAdd(vExtents, vOtherExtents);

    return XMVector3LessOrEqual(vDelta, vSum);
}

bool FBoundingVolume::IsIntersectSphere(const FBoundingVolume& Other) const
{
    XMVECTOR vCenter = XMLoadFloat3(&Center);
    XMVECTOR vOtherCenter = XMLoadFloat3(&Other.Center);

    XMVECTOR vDelta = XMVectorSubtract(vCenter, vOtherCenter);
    float distSquared = XMVectorGetX(XMVector3LengthSq(vDelta));

    //Extent.x를 반지름으로 사용
    float radiusSum = Extents.x + Other.Extents.x;
    return distSquared <= (radiusSum * radiusSum);
}

bool FBoundingVolume::IsIntersectCapsule(const FBoundingVolume& Other) const
{
    // 캡슐의 시작점과 끝점 계산
    Vector3 Start = Center - Vector3(0, Extents.y, 0);
    Vector3 End = Center + Vector3(0, Extents.y, 0);
    Vector3 OtherStart = Other.Center - Vector3(0, Other.Extents.y, 0);
    Vector3 OtherEnd = Other.Center + Vector3(0, Other.Extents.y, 0);

    // 선분 간 최단 거리 계산
    XMVECTOR vStart = XMLoadFloat3(&Start);
    XMVECTOR vEnd = XMLoadFloat3(&End);
    XMVECTOR vOtherStart = XMLoadFloat3(&OtherStart);
    XMVECTOR vOtherEnd = XMLoadFloat3(&OtherEnd);

    float segmentDistance = SegmentToSegmentDistance(vStart, vEnd, vOtherStart, vOtherEnd);
    float radiusSum = Extents.x + Other.Extents.x;

    return segmentDistance <= radiusSum;
}

float FBoundingVolume::SegmentToSegmentDistance(XMVECTOR A0, XMVECTOR A1, XMVECTOR B0, XMVECTOR B1) const
{
    XMVECTOR a = XMVectorSubtract(A1, A0);  // 방향 벡터
    XMVECTOR b = XMVectorSubtract(B1, B0);
    XMVECTOR c = XMVectorSubtract(B0, A0);

    float d1343 = XMVectorGetX(XMVector3Dot(c, b));
    float d4321 = XMVectorGetX(XMVector3Dot(b, a));
    float d1321 = XMVectorGetX(XMVector3Dot(c, a));
    float d4343 = XMVectorGetX(XMVector3Dot(b, b));
    float d2121 = XMVectorGetX(XMVector3Dot(a, a));

    float denom = d2121 * d4343 - d4321 * d4321;
    if (abs(denom) < KINDA_SMALL)
        return XMVectorGetX(XMVector3Length(c));

    float numer = d1343 * d4321 - d1321 * d4343;
    float mua = numer / denom;
    float mub = (d1343 + d4321 * mua) / d4343;

    // Clamp parameters to segment endpoints
    mua = Math::Clamp(mua, 0.0f, 1.0f);
    mub = Math::Clamp(mub, 0.0f, 1.0f);

    // Calculate closest points
    XMVECTOR pa = XMVectorAdd(A0, XMVectorScale(a, mua));
    XMVECTOR pb = XMVectorAdd(B0, XMVectorScale(b, mub));

    // Return distance between closest points
    return XMVectorGetX(XMVector3Length(XMVectorSubtract(pa, pb)));
}
#pragma endregion

#pragma region Collision

#pragma endregion

bool UCollisionComponent::CheckCollision(const UCollisionComponent* Other) const
{
    //상대방의 Extent를 자신의 모양과 같다고 가정하여 충돌 해석
    bool result = false;
    switch (Shape)
    {
        case ECollisionShape::Box:
        {
            result = BoundingVolume.IsIntersectBox(Other->GetBoudningVolume());
            break;
        }
        case ECollisionShape::Sphere:
        {
            result = BoundingVolume.IsIntersectSphere(Other->GetBoudningVolume());
            break;
        }
        case ECollisionShape::Capsule:
        {
            result = BoundingVolume.IsIntersectCapsule(Other->GetBoudningVolume());
            break;
        }
    }

    return result;
}

void UCollisionComponent::UpdateBounds()
{
    if (auto OwnerPtr = Owner.lock())
    {
        BoundingVolume.Center = OwnerPtr->GetTransform()->Position;
    }
    BoundingVolume.Extents = Extent;
}
