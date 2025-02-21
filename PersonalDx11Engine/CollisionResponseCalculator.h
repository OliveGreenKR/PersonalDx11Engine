#pragma once
#include "Math.h"
#include "CollisionDefines.h"

class FCollisionResponseCalculator
{
public:
    // 충돌 응답을 계산하는 주 함수
    FCollisionResponseResult CalculateResponseByImpulse(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    FCollisionResponseResult CalculateResponseByContraints(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );
private:
    struct FConstraintSolverCache
    {
        XMVECTOR vRelativeVel;
        XMVECTOR vNormal;
        float effectiveMass;
    };

private:
    // 충격량 기반 : 수직 충격량 계산 (탄성 충돌)
    XMVECTOR CalculateNormalImpulse(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    // 충격량 기반 : 마찰력에 의한 충격량 계산
    XMVECTOR CalculateFrictionImpulse(
        const XMVECTOR& NormalImpulse,
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    // 충격량 기반 : 상대 속도 계산
    XMVECTOR CalculateRelativeVelocity(
        const Vector3& ContactPoint,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB
    );

    FConstraintSolverCache PrepareConstraintCache(
        const FCollisionDetectionResult& DetectionResult,
        const FCollisionResponseParameters& ParameterA,
        const FCollisionResponseParameters& ParameterB);

    //제약 조건 기반 수직 항력 
    XMVECTOR SolveNormalConstraint(
        const FCollisionDetectionResult& DetectionResult,
        const FConstraintSolverCache& ConstraintCache,
        float& OutNormalLamda);

    //제약조건 기반 마찰력 
    XMVECTOR SolveFrictionConstraint(
        const FConstraintSolverCache& ConstraintCache,
        const float StaticFriction,
        const float KineticFriction,
        const float NormalLamda);

 
};

