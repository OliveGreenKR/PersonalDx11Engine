#pragma once


struct Vector3;
struct FTransform;

class IDynamicBoundable
{
public:
    virtual ~IDynamicBoundable() = default;
    virtual Vector3 GetScaledHalfExtent() const = 0;
    virtual Vector3 GetHalfExtent() const = 0;
    virtual const FTransform& GetWorldTransform() const = 0;
    virtual bool IsStatic() const = 0;
};