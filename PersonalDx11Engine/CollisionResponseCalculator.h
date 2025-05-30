#pragma once
#include "Math.h"
#include "CollisionDefines.h"
#include "ConstraintInterface.h"


class IPhysicsState;

class FCollisionResponseCalculator
{
public:

    // 법선 방향 충격량 계산 (반발 + 위치 편향 포함)
    Vector3 CalculateNormalImpulse(const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        float& InOutLambda,     // 초기 람다이자 출력  
        float BiasSpeed = 0.0f   // 외부에서 결정된 편향 속도
    ) const;

    // 마찰 방향 충격량 계산
    Vector3 CalculateFrictionImpulse(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        float NormalLambda,        // 법선 람다 (마찰 제한용)
        float& InOutFrictionLambda   // 초기 람다이자 출력
    ) const;

private:

    void ClampFriction(const float TangentRelativeVelocityLength,
        const float NormalLambda,
        const float StaticFriction,
        const float KineticFriction,
        float& OutFrictionLambda,
        Vector3& OutTangentImpulse) const;

 
};

