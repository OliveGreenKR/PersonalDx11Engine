#pragma once
///물리시스템 관리 객체
class IPhysicsObejct
{
public:
    virtual void TickPhysics(const float DeltaTime) = 0;

    virtual void RegisterPhysicsSystem() = 0;
    virtual void UnRegisterPhysicsSystem() = 0;

    // 내부 연산 결과를 외부용으로 복사
    virtual void SynchronizeCachedStateFromSimulated() = 0;
    //연산 전 외부 상태 연산용으로 복사
    virtual void UpdateSimulatedStateFromCached() = 0;

    //플래그 확인
    virtual bool IsDirtyPhysicsState() const = 0;

    virtual bool IsStatic() const = 0;
    virtual bool IsActive() const = 0;
    virtual bool IsSleep() const = 0;
};