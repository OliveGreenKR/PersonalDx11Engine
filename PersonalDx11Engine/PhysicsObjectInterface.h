#pragma once
///물리시스템 관리 객체
class IPhysicsObejct
{
public:
    virtual void TickPhysics(const float DeltaTime) = 0;

    virtual void RegisterPhysicsSystem() = 0;
    virtual void UnRegisterPhysicsSystem() = 0;

    //현재 상태를 외부 상태로 변경 (외부 상태 변화 반영)
    virtual void SynchronizeState() = 0;
    //외부 상태를  상태로 변경 (현재 상태 변화 반영)
    virtual void CaptureState() const = 0;

    //플래그 확인
    virtual bool IsDirtyPhysicsState() const = 0;

    virtual bool IsStatic() const = 0;
    virtual bool IsActive() const = 0;
};