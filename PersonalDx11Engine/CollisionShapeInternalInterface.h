#pragma once
#include <cstdint>
#include "Transform.h"

enum class ECollisionShapeType;
using PhysicsID = std::uint32_t;

/// <summary>
/// 충돌 형상 정보 물리 인터페이스
/// </summary>
class ICollisionShapeInternal
{
public:
    // 형상 타입 관리
    virtual ECollisionShapeType P_GetShapeType(PhysicsID id) const = 0;
    virtual void P_SetShapeType(PhysicsID id, ECollisionShapeType type) = 0;

    // 형상 크기 (HalfExtent 기반)
    virtual Vector3 P_GetShapeHalfExtent(PhysicsID id) const = 0;
    virtual void P_SetShapeHalfExtent(PhysicsID id, const Vector3& extent) = 0;

    // 트랜스폼 반환
    virtual FTransform P_GetCurrentWorldTransform(PhysicsID id) const = 0;
    virtual void P_SetCurrentWorldTransform(PhysicsID id, const FTransform& transform) = 0;

    // 이전 트랜스폼 반환
    virtual FTransform P_GetPrevWorldTransform(PhysicsID id) const = 0;
    virtual void P_SetPrevWorldTransform(PhysicsID id, const FTransform& transform) = 0;

};