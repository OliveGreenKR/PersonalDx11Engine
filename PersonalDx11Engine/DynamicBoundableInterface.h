#pragma once

struct Vector3;
struct FTransform;

class IDynamicBoundable
{
public:
    virtual ~IDynamicBoundable() = default;
    virtual Vector3 GetHalfExtent() const = 0;
    virtual const FTransform* GetTransform() const = 0;

    // 이전 프레임의 AABB와 현재 AABB를 비교하여 변경 여부 확인
    virtual bool HasBoundsChanged() const = 0;
};