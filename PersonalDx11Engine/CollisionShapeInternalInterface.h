#pragma once
#include <DirectXMath.h>
#include "PhysicsDefine.h"
#include "Math.h"
#include "Transform.h"

using namespace DirectX;
using PhysicsID = std::uint32_t;

enum class ECollisionShapeType;

/// <summary>
/// 충돌 형상 정보 물리 내부 인터페이스
/// 
/// 책임:
/// - 충돌 형상의 기하학적 정보를 물리 시스템 내부에서 관리
/// - 형상 타입, 크기 정보에 대한 CRUD 인터페이스 제공
/// - SIMD 최적화된 XMVECTOR와 게임 로직용 Vector3 Setter 모두 지원
/// - 물리 시뮬레이션에서 사용하는 충돌 형상 데이터의 추상화 레이어
/// </summary>
class ICollisionShapeInternal
{
public:
    virtual ~ICollisionShapeInternal() = default;

    // === 형상 타입 관리 ===
    virtual ECollisionShapeType P_GetShapeType(PhysicsID id) const = 0;
    virtual void P_SetShapeType(PhysicsID id, ECollisionShapeType type) = 0;

    // === 형상 크기 (XMVECTOR 가상 함수) ===
    virtual XMVECTOR P_GetShapeHalfExtent(PhysicsID id) const = 0;
    virtual void P_SetShapeHalfExtent(PhysicsID id, XMVECTOR extent) = 0;

    // === 형상 크기 설정 (Vector3 오버로드) ===
    void P_SetShapeHalfExtent(PhysicsID id, const Vector3& extent)
    {
        XMVECTOR extentVec = XMVectorSet(extent.x, extent.y, extent.z, 0.0f);
        P_SetShapeHalfExtent(id, extentVec);
    }

    // === 현재 프레임 월드 트랜스폼 (XMVECTOR 가상 함수) ===
    virtual XMVECTOR P_GetWorldPosition(PhysicsID id) const = 0;
    virtual XMVECTOR P_GetWorldRotationQuat(PhysicsID id) const = 0;
    virtual XMVECTOR P_GetWorldScale(PhysicsID id) const = 0;

    // === 이전 프레임 월드 트랜스폼 (XMVECTOR 가상 함수) ===
    virtual XMVECTOR P_GetPrevWorldPosition(PhysicsID id) const = 0;
    virtual XMVECTOR P_GetPrevWorldRotationQuat(PhysicsID id) const = 0;
    virtual XMVECTOR P_GetPrevWorldScale(PhysicsID id) const = 0;
};