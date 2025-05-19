#pragma once
///물리시스템 관리 객체
class IPhysicsObejct
{
public:
    virtual void TickPhysics(const float DeltaTime) = 0;

    virtual bool IsStatic() const = 0;
    virtual bool IsActive() const = 0;

    virtual void SynchronizeState() = 0;
    virtual void CaptureState() const = 0;
    virtual bool IsDirty() const = 0;
};