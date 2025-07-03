#pragma once

using PhysicsID = uint32_t;

///물리시스템 관리 객체
class IPhysicsObject
{
public:
    virtual void TickPhysics(const float DeltaTime) = 0;

    virtual void RegisterPhysicsSystem() = 0;
    virtual void UnRegisterPhysicsSystem() = 0;

    // 내부 연산 결과를 외부용으로 복사
    virtual void SynchronizeCachedStateFromSimulated() = 0;

    //상태 확인
    virtual struct FPhysicsMask GetPhysicsMask() const = 0;

    virtual PhysicsID GetPhysicsID() const = 0;
    virtual void SetPhysicsID(PhysicsID InID) = 0;
};