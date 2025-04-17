#pragma once
#include "Math.h"
#include "Transform.h"

class ICollisionShape
{
public:
    virtual ~ICollisionShape() = default;

    // 지원 함수 (Support function) - 주어진 방향의 가장 먼 점
    virtual Vector3 GetSupportPoint(const Vector3& Direction, const FTransform& WorldTransform) const = 0;

    // 관성 텐서 계산 (질량 기반)
    virtual Vector3 CalculateInertiaTensor(float Mass) const = 0;

    // AABB 계산 (Axis-Aligned Bounding Box)
    virtual void CalculateAABB(const FTransform& WorldTransform, Vector3& OutMin, Vector3& OutMax) const = 0;

    virtual const Vector3& GetHalfExtent() const = 0;
    virtual void SetHalfExtent(const Vector3& InVector) = 0;
};