#pragma once
#include "Math.h"
#include "Transform.h"

enum class ECollisionShapeType;
/// <summary>
/// 충돌 형상 정보 게임 인터페이스
/// </summary>
class ICollisionShape
{
public:
    virtual ~ICollisionShape() = default;

    // 지원 함수 (Support function) - 주어진 월드 방향의 가장 먼 로컬 좌표 점
    virtual Vector3 GetWorldSupportPoint(const Vector3& WorldDirection) const = 0;

    // 관성 텐서 계산 (질량 기반)
    virtual Vector3 CalculateInvInertiaTensor(float InvMass) const = 0;

    virtual Vector3 GetScaledHalfExtent() const = 0;
    virtual Vector3 GetLocalHalfExtent() const = 0;
    virtual void SetHalfExtent(const Vector3& InVector) = 0;

    virtual ECollisionShapeType GetType() const = 0;
};