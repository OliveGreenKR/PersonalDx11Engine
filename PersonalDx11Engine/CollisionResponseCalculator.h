#pragma once
#include "Math.h"
#include "CollisionDefines.h"
#include "ConstraintInterface.h"

class FCollisionResponseCalculator
{
public:

    FCollisionResponseResult CalculateResponseByContraints(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        FAccumulatedConstraint& Accumulation
    );
private:
    struct FConstraintSolverCache
    {
        XMVECTOR vRelativeVel;
        XMVECTOR vNormal;
        float effectiveMass;
    };

private:

    // 상대 속도 계산
    XMVECTOR CalculateRelativeVelocity(
        const Vector3& ContactPoint,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB
    );

    FConstraintSolverCache PrepareConstraintCache(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB);

    //제약 조건 기반 수직 항력 
    XMVECTOR SolveNormalConstraint(
        const FCollisionDetectionResult& DetectionResult,
        const FConstraintSolverCache& ConstraintCache,
        FAccumulatedConstraint& Accumulation);

    //제약조건 기반 마찰력 
    XMVECTOR SolveFrictionConstraint(
        const FConstraintSolverCache& ConstraintCache,
        const float StaticFriction,
        const float KineticFriction,
        FAccumulatedConstraint& Accumulation);

 
};

