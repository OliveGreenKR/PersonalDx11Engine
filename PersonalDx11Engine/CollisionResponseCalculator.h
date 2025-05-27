#pragma once
#include "Math.h"
#include "CollisionDefines.h"
#include "ConstraintInterface.h"


class IPhysicsState;

class FCollisionResponseCalculator
{
public:

    FCollisionResponseResult CalculateResponseByContraints(const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        FAccumulatedConstraint& Accumulation,
        const float DeltaTime
    );

private:

    // 법선 방향 충돌 제약 조건 해결
    Vector3 SolveNormalCollisionConstraint(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        FAccumulatedConstraint& Accumulation,
        const float DeltaTime,
        float& OutNormalLambda);

    // 접선 방향 마찰 제약 조건 해결
    Vector3 SolveFrictionConstraint(
        const FCollisionDetectionResult& DetectionResult,
        const FPhysicsParameters& ParameterA,
        const FPhysicsParameters& ParameterB,
        FAccumulatedConstraint& Accumulation,
        float InNormalLambda, // 법선 임펄스 크기에 따라 마찰 클램핑 필요
        float& OutFrictionLambda);

    // 위치 오류를 속도 편향으로 변환하는 헬퍼 함수
    // 슬롭 단위 : m
    float CalculatePositionBiasVelocity(
        float PenetrationDepth,
        float BiasFactor,
        const float DeltaTime,
        float Slop = 0.05f); 


    void ClampFriction(const float TangentRelativeVelocityLength,
        const float NormalLambda,
        const float StaticFriction,
        const float KineticFriction,
        float& OutFrictionLambda,
        Vector3& OutTangentImpulse);

 
};

