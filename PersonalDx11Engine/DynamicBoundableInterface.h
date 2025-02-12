#pragma once


struct Vector3;
struct FTransform;

class IDynamicBoundable
{
public:
    virtual ~IDynamicBoundable() = default;
    virtual Vector3 GetHalfExtent() const = 0;
    virtual const FTransform* GetTransform() const = 0;
    virtual bool IsStatic() const = 0;
};